# Deribit Trading System

This project is a comprehensive trading system for the Deribit cryptocurrency exchange, implemented in C++. It provides functionalities for connecting to Deribit (both testnet and mainnet), authenticating, and executing a variety of public and private API calls, such as order placement, order management, subscription to market data, and more. The system uses WebSocket communication to ensure real-time interaction with the Deribit exchange.

## Features

- **Connection Management**: Connect to Deribit testnet or mainnet.
- **Authentication**: Authenticate using Deribit API credentials.
- **Public API Access**:
  - Get server time.
  - Retrieve available instruments and currencies.
  - Fetch order books and trading view chart data.
- **Private API Access**:
  - Get account summary and positions.
  - Place, modify, and cancel orders.
  - Retrieve open orders and order history.
- **Subscriptions**:
  - Subscribe to order book updates, trade streams, and ticker updates.
- **Command-Line Interface (CLI)**: User-friendly CLI for managing trading and market data interactions.

## Dependencies

The project relies on the following libraries:

- **Boost**
- **WebSocket++**
- **nlohmann/json**
- **OpenSSL**
- **cURL**

### Installing Dependencies

You can manage dependencies using [vcpkg](https://vcpkg.io/). Ensure you have `vcpkg` installed and set up correctly on your system. Follow these steps:

1. Clone the vcpkg repository (if not already done):
   ```sh
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # or .\bootstrap-vcpkg.bat for Windows
   ```

2. Install the required libraries:
   ```sh
   ./vcpkg install boost websocketpp nlohmann-json openssl curl
   ```

3. Integrate vcpkg with your development environment (optional):
   ```sh
   ./vcpkg integrate install
   ```

## Building the Project

1. Clone the repository:
   ```sh
   git clone <repository-url>
   cd <repository-name>
   ```

2. Ensure CMake is installed on your system.

3. Configure the project:
   ```sh
   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
   ```

4. Build the project:
   ```sh
   cmake --build build
   ```

## Running the Application

1. Navigate to the build directory:
   ```sh
   cd build
   ```

2. Run the executable:
   ```sh
   ./DeribitTradingSystem  # or DeribitTradingSystem.exe for Windows
   ```

3. Follow the on-screen prompts to connect to the Deribit exchange, authenticate, and start trading.

## Usage

The CLI interface allows you to interact with the system easily. Type `help` to see the list of available commands.

## Project Structure

- **src**: Contains the main source code, including:
  - `main.cpp`: Entry point of the application.
  - `DeribitFullTrader`: Core class implementing the trading functionalities.

## License

MIT License

Copyright (c) 2024 PARTH SHEKHAWAT

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## Acknowledgments

Special thanks to the authors of Boost, WebSocket++, nlohmann/json, OpenSSL, and cURL for providing essential libraries that power this project.

