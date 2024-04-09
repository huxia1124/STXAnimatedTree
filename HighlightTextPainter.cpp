#include "stdafx.h"
#include "HighlightTextPainter.h"
#include <regex>
#include <vector>

template<typename TString>
bool ParseTokens(const typename TString::value_type* text, const typename TString::value_type* separators, bool removeEmptyTokens, std::vector<typename TString>& outResult)
{
	static_assert(std::is_base_of<std::basic_string<char>, TString>::value || std::is_base_of<std::basic_string<wchar_t>, TString>::value, "TString must be std::string or std::wstring");

	outResult.clear();

	// Empty separator list is not allowed
	if (separators == nullptr || separators[0] == 0)
		return false;

	// Quotation mark can not be used as a separator
	const typename TString::value_type* sep = separators;
	while (*sep)
	{
		if (*sep == '\"')
			return false;

		++sep;
	}

	auto isSeparator = [separators](typename TString::value_type ch)
	{
		const typename TString::value_type* sep = separators;
		while (*sep)
		{
			if (*sep == ch)
			{
				return true;
			}

			++sep;
		}
		return false;
	};

	enum State
	{
		Token = 1,
		Quote = 2,
	};

	State state = Token;
	const typename TString::value_type* p = text;
	const typename TString::value_type* lastPos = p;
	while (true)
	{
		switch (state)
		{
		case Token:
			if (!*p)
			{
				if (p != lastPos || !removeEmptyTokens)
				{
					outResult.emplace_back(lastPos, p);
				}
				return true;
			}
			else if (*p == '\"')
			{
				state = Quote;
			}
			else if (isSeparator(*p))
			{
				if (p != lastPos || !removeEmptyTokens)
				{
					outResult.emplace_back(lastPos, p);
				}
				lastPos = p + 1;
			}
			break;
		case Quote:	// in quote
			if (!*p)
			{
				outResult.clear();
				return false;
			}
			else if (*p == '\"')
			{
				state = Token;
			}
			break;
		}

		if (!*p)
		{
			break;
		}

		++p;
	}

	return true;
}

// trim from start (in place)
static inline void ltrim(std::wstring& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
		return !std::isspace(ch) && ch != '\"';
		}));
}

// trim from end (in place)
static inline void rtrim(std::wstring& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
		return !std::isspace(ch) && ch != '\"';
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::wstring& s) {
	ltrim(s);
	rtrim(s);
}

void HighlightTextPainter::DefaultSplitter::Split(const wchar_t* text, const wchar_t* keywords, std::list<std::pair<size_t, bool>>& startIndice)
{
	if (keywords[0] == '\0')
	{
		startIndice.emplace_back(0, false);
		return;
	}

	std::vector<std::wstring> tokens;
	ParseTokens(keywords, L" ", true, tokens);

	std::wstring rgxExpression;
	for (auto its = tokens.begin(); its != tokens.end(); ++its)
	{
		trim(*its);
		rgxExpression += *its;

		if (its + 1 != tokens.end())
		{
			rgxExpression += L"|";
		}
	}

	std::wstring s(text);
	std::wregex rgx(rgxExpression, _caseSensitive ? std::wregex::ECMAScript : std::wregex::icase);

	std::wsregex_iterator end;
	std::wsregex_iterator last;
	std::wsregex_iterator it(s.begin(), s.end(), rgx, std::regex_constants::match_any);
	for (; it != end; ++it)
	{
		last = it;
		const std::wsmatch match = *it;

		startIndice.emplace_back(match.prefix().first - s.begin(), false);
		startIndice.emplace_back(match.position(), true);
	}

	if (last != end && (*last).suffix().length() > 0)
	{
		startIndice.emplace_back((*last).suffix().first - s.begin(), false);
	}

	if (startIndice.empty())
	{
		// No matched text.
		startIndice.emplace_back(0, false);
	}
}

HighlightTextPainter::HighlightTextPainter(const wchar_t* text, const wchar_t* keyword, bool multiLine)
	: _multiLineText(multiLine)
	, _text(text)
{
	DefaultSplitter sp;
	sp.Split(text, keyword, _seg);

	// ----- Debugging Info -----
	//for (auto it = _seg.begin(); it != _seg.end(); ++it)
	//{
	//	auto itNext = it;
	//	++itNext;
	//	int len = itNext == _seg.end() ? _text.length() - it->first : itNext->first - it->first;

	//	std::wstring s(text + it->first, len);

	//	wchar_t buf[20];
	//	_itow_s(it->first, buf, 10);
	//	OutputDebugStringW(buf);
	//	OutputDebugStringW(_T("|"));
	//	OutputDebugStringW(s.c_str());
	//	OutputDebugStringW(_T("\r\n"));
	//}
}

HighlightTextPainter::HighlightTextPainter(ITextSplitter* splitter, const wchar_t* text, const wchar_t* keyword, bool multiLine)
	: _multiLineText(multiLine)
	, _text(text)
{
	splitter->Split(text, keyword, _seg);
}

void HighlightTextPainter::Draw(ITextPainter* painter, float x, float y, float w, float h, bool selected)
{
	FindLineBreaks(painter, w, h);

	std::transform(_seg.begin(), _seg.end(), std::back_inserter(_dynseg), [](const std::pair<size_t, bool>& v)
		{
			std::pair<size_t, TextInfo> result;
			result.first = v.first;
			result.second.highlight = v.second;
			result.second.offsetX = 0;
			result.second.offsetY = 0;
			return result;
		});

	ApplyLineBreaks(painter, w, h);

	for (auto it = _dynseg.begin(); it != _dynseg.end(); ++it)
	{
		auto itNext = it;
		++itNext;
		size_t len = itNext == _dynseg.end() ? _text.length() - it->first : itNext->first - it->first;
		painter->DrawText(_text.c_str() + it->first, static_cast<int>(len), it->second.offsetX + x, it->second.offsetY + y, w, h, it->second.highlight, selected);
	}
}

int HighlightTextPainter::GetNumCharsInLine(ITextPainter* painter, const wchar_t* text, int len, float w)
{
	float cx = 0;
	float cy = 0;
	int r = 0;
	painter->MeasureText(text, len, cx, cy);
	if (cx > w)
	{
		// Binary search to get the number of characters this line can hold
		int start = 1;
		int end = len - 1;
		int mid = (start + end) / 2;
		while (start < end)
		{
			mid = (start + end) / 2;
			painter->MeasureText(text, mid, cx, cy);
			if (cx == w)
			{
				break;
			}
			else if (cx < w)
			{
				painter->MeasureText(text, mid + 1, cx, cy);
				if (cx > w)
				{
					break;
				}
				start = mid + 1;
			}
			else
				end = mid - 1;
		}

		// Go back to the beginning of the current word
		while (mid > 0 && isalpha(text[mid - 1]))
			--mid;

		// Continue going back until we reach the previous word
		while (mid > 0 && isspace(text[mid - 1]))
			--mid;

		if (mid == 0)	// No previous word
		{
			// Make it at least one word per line
			while (mid < len && isalpha(text[mid]))
				++mid;
		}

		// Now try to include the spaces after the last word in this line
		while (mid < len && !isalpha(text[mid]))
			++mid;

		r = mid;
	}
	else
	{
		r = len;
	}

	// Look for the first explicit '\n' in this line. If found, use it as line-break
	int hardBreak = 0;
	while (hardBreak < r)
	{
		if (text[hardBreak] == '\n')
		{
			while (hardBreak + 1 < r && isspace(text[hardBreak + 1]) && text[hardBreak + 1] != '\n')
				++hardBreak;

			return hardBreak + 1;
		}

		++hardBreak;
	}
	return r;
}

void HighlightTextPainter::FindLineBreaks(ITextPainter* painter, float w, float h)
{
	if (!_multiLineText)
	{
		return;
	}

	_lineBreaks.clear();
	size_t current = 0;
	int num = GetNumCharsInLine(painter, _text.c_str(), static_cast<int>(_text.size()), w);
	while (num + current < _text.size())
	{
		current += num;
		num = GetNumCharsInLine(painter, _text.c_str() + current, static_cast<int>(_text.size() - current), w);
		_lineBreaks.push_back(current);
	}
}

void HighlightTextPainter::ApplyLineBreaks(ITextPainter* painter, float w, float h)
{
	float offsetX = 0;
	float offsetY = 0;
	auto itBreak = _lineBreaks.begin();
	for (auto it = _dynseg.begin(); it != _dynseg.end();)
	{
		auto itNext = it;
		++itNext;

		it->second.offsetX = offsetX;
		it->second.offsetY = offsetY;
		size_t len = itNext == _dynseg.end() ? _text.length() - it->first : itNext->first - it->first;

		float cx = 0;
		float cy = 0;
		painter->MeasureText(_text.c_str() + it->first, static_cast<int>(len), cx, cy);

		// if there is a line-break in this segment
		if (itBreak != _lineBreaks.end() && it->first <= *itBreak && it->first + len >= *itBreak)
		{
			// Break this segment into two and put them in the list, also remove the original one.
			std::pair<size_t, TextInfo> node1;
			node1.first = it->first;
			node1.second.offsetX = offsetX;
			node1.second.offsetY = offsetY;
			node1.second.highlight = it->second.highlight;

			std::pair<size_t, TextInfo> node2;
			node2.first = *itBreak;
			node2.second.offsetX = 0;
			node2.second.highlight = it->second.highlight;

			_dynseg.erase(it);

			_dynseg.insert(itNext, node1);

			offsetX = 0;
			offsetY += cy;
			node2.second.offsetY = offsetY;

			it = _dynseg.insert(itNext, node2);
			++itBreak;
		}
		else
		{
			offsetX += cx;
			++it;
		}
	}
}

void HighlightTextPainter::GDIPainter::MeasureText(const wchar_t* text, int len, float& cx, float& cy)
{
	SIZE size = {};
	GetTextExtentPoint32W(_hdc, text, len, &size);
	cx = static_cast<float>(size.cx);
	cy = static_cast<float>(size.cy);
}

void HighlightTextPainter::GDIPainter::DrawText(const wchar_t* text, int len, float x, float y, float w, float h, bool highlight, bool selected)
{
	if (len <= 0)
		return;

	const DWORD clrHighlightText = RGB(255, 0, 0);
	const DWORD clrHighlightBk = RGB(255, 255, 32);
	const DWORD clrHighlightSelectedText = RGB(255, 255, 0);

	DWORD textClr = GetSysColor(COLOR_WINDOWTEXT);
	DWORD highlightBkCr = clrHighlightBk;
	DWORD highlightClr = clrHighlightText;

	if (selected)
	{
		textClr = GetSysColor(COLOR_HIGHLIGHTTEXT);
		highlightBkCr = GetSysColor(COLOR_MENUHILIGHT) ^ 0x00FFFFFF;
		highlightClr = clrHighlightSelectedText;
	}

	int oldBkMode = ::GetBkMode(_hdc);
	COLORREF oldBkColor = ::GetBkColor(_hdc);
	COLORREF oldTextColor = ::GetTextColor(_hdc);

	if (highlight)
	{
		::SetBkMode(_hdc, OPAQUE);
		::SetTextColor(_hdc, highlightClr);
		::SetBkColor(_hdc, highlightBkCr);
	}
	else
	{
		::SetBkMode(_hdc, TRANSPARENT);
		::SetTextColor(_hdc, textClr);
	}

	RECT rc = { static_cast<int>(x), static_cast<int>(y), static_cast<int>(x + w), static_cast<int>(y + h) };

	// No deed to draw '\n' and spaces after '\n'
	int pos = len - 1;
	while (pos > 0 && isspace(text[pos]) && text[pos] != '\n')
		--pos;
	if (text[pos] == '\n')
		len = pos;

	if (len > 0)
	{
		::DrawText(_hdc, text, len, &rc, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX);
	}

	::SetBkMode(_hdc, oldBkMode);
	::SetBkColor(_hdc, oldBkColor);
	::SetTextColor(_hdc, oldTextColor);
}

#ifdef _WIN32		// Windows (x64 and x86)
void HighlightTextPainter::GDIPlusPainter::MeasureText(const wchar_t* text, int len, float& cx, float& cy)
{
	Gdiplus::RectF rectTextMain;
	Gdiplus::RectF rectTextMeasured;
	_graphics.MeasureString(text, len, &_font, rectTextMain, &_format, &rectTextMeasured);

	cx = rectTextMeasured.Width;
	cy = rectTextMeasured.Height;
}

void HighlightTextPainter::GDIPlusPainter::DrawText(const wchar_t* text, int len, float x, float y, float w, float h, bool highlight, bool selected)
{
	if (len <= 0)
		return;

	// No deed to draw '\n' and spaces after '\n'
	int pos = len - 1;
	while (pos > 0 && isspace(text[pos]) && text[pos] != '\n')
		--pos;
	if (text[pos] == '\n')
		len = pos;

	if (len <= 0)
		return;

	const DWORD clrHighlightText = RGB(255, 0, 0);
	const DWORD clrHighlightBk = RGB(255, 255, 32);
	const DWORD clrHighlightSelectedText = RGB(255, 255, 0);

	DWORD textClr = GetSysColor(COLOR_WINDOWTEXT);
	DWORD highlightBkCr = clrHighlightBk;
	DWORD highlightClr = clrHighlightText;

	if (selected)
	{
		textClr = GetSysColor(COLOR_HIGHLIGHTTEXT);
		highlightBkCr = GetSysColor(COLOR_MENUHILIGHT) ^ 0x00FFFFFF;
		highlightClr = clrHighlightSelectedText;
	}

	Gdiplus::RectF rectTextMain(x, y, w, h);
	if (highlight)
	{
		float cx, cy;
		MeasureText(text, len, cx, cy);
		Gdiplus::RectF rectTextBk(x, y, cx, cy);

		Gdiplus::SolidBrush bkBrush(Gdiplus::Color(static_cast<BYTE>(_alpha), GetRValue(highlightBkCr), GetGValue(highlightBkCr), GetBValue(highlightBkCr)));
		_graphics.FillRectangle(&bkBrush, rectTextBk);

		Gdiplus::SolidBrush textBrush(Gdiplus::Color(static_cast<BYTE>(_alpha), GetRValue(highlightClr), GetGValue(highlightClr), GetBValue(highlightClr)));
		_graphics.DrawString(text, len, &_font, rectTextMain, &_format, &textBrush);
	}
	else
	{
		Gdiplus::SolidBrush textBrush(Gdiplus::Color(static_cast<BYTE>(_alpha), GetRValue(textClr), GetGValue(textClr), GetBValue(textClr)));
		_graphics.DrawString(text, len, &_font, rectTextMain, &_format, &textBrush);
	}
}
#endif