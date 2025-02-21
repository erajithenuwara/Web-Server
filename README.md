# Simple HTTP Web Server in C

This project is a lightweight, multi-threaded HTTP web server implemented in C. It listens for incoming client requests, serves static files (HTML, CSS, JS, images, etc.), and handles errors such as 404 (Not Found) and 400 (Bad Request). The server supports concurrent connections using a thread pool and dynamically determines MIME types for proper content delivery.

## Key Features
- **Multi-threaded:** Handles multiple clients concurrently using a thread pool.
- **Static File Serving:** Serves files from a predefined directory (`./www/`).
- **Error Handling:** Returns appropriate HTTP error responses (e.g., 404, 400).
- **MIME Type Detection:** Automatically determines the correct `Content-Type` for files.
- **Platform:** Designed for Windows using Winsock for socket programming.

## How It Works
1. The server listens on port `8080` for incoming HTTP requests.
2. It parses the request to determine the requested file.
3. If the file exists, it sends the file with the appropriate headers.
4. If the file doesn't exist or the request is invalid, it sends an error response.

## Technologies Used
- **C Programming Language**
- **Winsock API** for socket programming
- **Multi-threading** for concurrent connections

## How to Run
1. Clone the repository.
2. Compile the code using a C compiler (e.g., `gcc`).
3. Place your static files (HTML, CSS, etc.) in the `./www/` directory.
4. Run the server executable.
5. Access the server via [http://localhost:8080](http://localhost:8080) in your browser.
