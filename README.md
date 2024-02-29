# TinyLogger

This is a tiny header-only c++ logger. 

## Features

Two concrete loggers (`StdoutLogger` & `FileLogger`) are provided currently. You can add your own ones by simply extends `Logger`. Features provided:
- Thread-safe
- Log levels
- For StdoutLogger
    - Colored output
- For FileLogger
    - Log file size limit

 ## Build & Test

 Simply build the main.cpp and test
 
 ```
 g++ main.cpp
```
