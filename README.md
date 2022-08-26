# STXAnimatedTree
A tree control with animation. Pure Windows API implementation from scratch. (built around the year 2012)

This animated tree control follows the API pattern as the standard tree control. Common event notifications are also implemented. Integated with MSAA, making it ready for UI test automation.

## Why do the member functions have "Internal_" prefix?
## Because the implementation was not complete. My initial intention was to make those member functions thread-safe but did nothave time to finish. The thread-safe member functions will send a message to the window handle, rather than changing its member variables directly. 


![Demo](https://github.com/huxia1124/STXAnimatedTree/blob/master/Screenshots/Demo.gif)
