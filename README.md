# COM Port Logger

A sophisticated Windows COM port logger with real-time CLI input, intelligent console management, and configurable baud rates.

## Features

- **Multi-threaded Design**: Simultaneous COM port monitoring and user input
- **Smart Line Buffering**: Only displays complete lines or when buffer is full
- **Sophisticated Console**: Clean line management that preserves user input while displaying COM data
- **Device Discovery**: Lists available COM devices with descriptions
- **Configurable Baud Rates**: Choose from common rates (9600-921600) or set custom values
- **File Logging**: Optional timestamped logging to file
- **Thread-Safe**: Robust mutex-based synchronization

## Usage

0. **Compile using Makefile**
   ```
   make          # Build the project
   ```

1. **Start the program**
   ```
   ./comlogger.exe
   ```

2. **Device Discovery**
   - Program automatically lists available COM devices
   - Shows both device descriptions and port availability

3. **Connect to COM Port**
   - Enter desired COM port number (1-20)
   - Select baud rate from menu:
     - Common rates: 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600
     - Custom rate: Enter any value
   - Program establishes connection and starts logging

4. **Commands**
   - Type any text and press Enter to send to COM port
   - `quit` - Exit the program
   - `log on` - Enable file logging (creates timestamped file)
   - `log off` - Disable file logging
   - `baud` - Display current baud rate
   - Press ESC to quit

## Interface

```
[2025-07-12 14:30:15] RX: Sensor data: 25.3°C
[2025-07-12 14:30:16] TX: status
[2025-07-12 14:30:16] RX: Device OK
> hello wor█
```

The interface intelligently manages console output:
- COM data appears immediately with timestamps
- User input line is preserved and restored
- Clean separation between received and transmitted data

## File Structure

```
├── main.cpp                    # Main application
├── SerialPort/                 # SerialPort library folder
│   ├── SerialPort.hpp         # Serial port header
│   └── SerialPort.cpp         # Serial port implementation
├── Makefile                   # Build configuration
└── README.md                  # This file
```

## Configuration

- **Baud Rate**: User selectable (9600-921600 + custom)
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Buffer Size**: 512 bytes
- **Max Input**: 256 characters

## Technical Details

- Uses Windows Console API for reliable console management
- Thread-safe operations with mutex synchronization
- Line-based buffering for complete data integrity
- Automatic device scanning using WMI queries
- Graceful cleanup on exit

## Error Handling

- Connection failure detection
- Port access denied handling
- Write operation error reporting
- Thread synchronization safety
- Graceful shutdown procedures

## License

MIT License - Feel free to modify and distribute.
