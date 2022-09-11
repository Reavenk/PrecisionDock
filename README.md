# AppDock

No description ATM.

This application is not in a usable state. Play with it at your own risk.

# Codebase

The code is designed for Windows, compiled with Visual Studio. Visual Studio 2019 is current target compiler.

The repo requires wxWidgets 3.1.3 to be installed. To specify the location of it to the Visual Studio project, copy the file `AppDock/PerDeveloper._props` and rename it to `PreDeveloper.props`. Open the copied file in a text editor and change the value for the macro `WX_WIDGETS`.

# License

Copyright 2022 Pixel Precision (William Leu)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.