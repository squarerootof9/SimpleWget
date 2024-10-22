/*
MIT License

Copyright (c) 2024

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

[Full MIT License Text]
*/

#include <windows.h>
#include <winhttp.h>
#include <wininet.h>  // For FTP (if needed)
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <regex>
#include <sstream>  // For encoding to Base64

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")  // Link WinINet for FTP (if needed)

// Global variables
bool ignoreSSLCerts = false;
bool noClobber = false;
int tries = 1;
bool unlimitedRetries = false;
int timeout = 30000;
int maxRedirects = 5;
bool httpsOnly = false;
bool serverResponse = false;
bool debugMode = false;
std::string secureProtocol = "auto";
std::string httpUser = "";
std::string httpPassword = "";
std::string ftpUser = "anonymous";
std::string ftpPassword = "";

// Function to encode credentials in Base64
std::string Base64Encode(const std::string& input) {
    static const char encodeLookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::stringstream encoded;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded << encodeLookup[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    if (valb > -6) encoded << encodeLookup[((val << 8) >> (valb + 8)) & 0x3F];
    while (encoded.str().size() % 4) encoded << '=';
    return encoded.str();
}

// Function to add the Authorization header for basic auth
void AddBasicAuthHeader(HINTERNET hRequest, const std::string& user, const std::string& password) {
    if (!user.empty() && !password.empty()) {
        std::string credentials = user + ":" + password;
        std::string encodedCredentials = Base64Encode(credentials);
        std::string header = "Authorization: Basic " + encodedCredentials;
        std::wstring wheader(header.begin(), header.end());
        WinHttpAddRequestHeaders(hRequest, wheader.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    }
}

// Function to show version information
void ShowVersion() {
    std::cout << "Simple Wget version 1.2.6 (compiled on " << __DATE__ << " " << __TIME__ << ")\n";
    std::cout << "MIT License: Permission is hereby granted, free of charge, to any person obtaining a copy of this software.\n";
}

// Function to show help information
void ShowHelp() {
    std::cout << "Usage: s_wget [options] <URL>\n\n";
    std::cout << "Startup:\n";
    std::cout << "  -V, --version              display this version of s_wget.exe and exit\n";
    std::cout << "  -h, --help                 print this help\n\n";
    std::cout << "Download:\n";
    std::cout << "  -nc, --no-clobber          skip downloads that would overwrite existing files\n";
    std::cout << "  -t, --tries=NUMBER         set number of retries to NUMBER (0 unlimits)\n";
    std::cout << "  -O, --output-document=FILE write document to FILE\n";
    std::cout << "  -S, --server-response      print server response\n";
    std::cout << "  -d, --debug                print lots of debugging information\n\n";
    std::cout << "HTTP options:\n";
    std::cout << "  --http-user=USER           set http user to USER\n";
    std::cout << "  --http-password=PASS       set http password to PASS\n\n";
    std::cout << "HTTPS (SSL/TLS) options:\n";
    std::cout << "  --no-check-certificate     disable SSL certificate checking\n";
    std::cout << "  --https-only               only follow secure HTTPS links\n";
    std::cout << "  --secure-protocol=PR       choose secure protocol, one of auto, TLSv1, TLSv1_1, TLSv1_2\n\n";
    std::cout << "FTP options:\n";
    std::cout << "  --ftp-user=USER            set ftp user to USER\n";
    std::cout << "  --ftp-password=PASS        set ftp password to PASS\n";
}

// Function to extract the filename from a URL
std::wstring GetFileNameFromURL(const std::wstring& url) {
    std::wregex fileRegex(L"[^/]+$");
    std::wsmatch match;
    if (std::regex_search(url, match, fileRegex)) {
        return match.str();
    }
    return L"default.out";
}

// Function to check if a file exists
bool FileExists(const std::wstring& fileName) {
    struct _stat buffer;
    return (_wstat(fileName.c_str(), &buffer) == 0);
}

// Function to print server response headers
void PrintServerResponse(HINTERNET hRequest) {
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &size, NULL)) {
        std::wcout << L"Server Response: Status Code = " << statusCode << L"\n";
    } else {
        std::cerr << "Error: Unable to query status code.\n";
    }

    // Retrieve and print all headers
    DWORD headerBufferSize = 0;
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &headerBufferSize, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        WCHAR* headerBuffer = new WCHAR[headerBufferSize / sizeof(WCHAR)];
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, headerBuffer, &headerBufferSize, WINHTTP_NO_HEADER_INDEX)) {
            std::wcout << headerBuffer << L"\n";
        } else {
            std::cerr << "Error: Unable to retrieve headers.\n";
        }
        delete[] headerBuffer;
    } else {
        std::cerr << "Error: Unable to query headers.\n";
    }
}

// Function to enable modern TLS/SSL protocols
void EnableTLS(HINTERNET hSession) {
    DWORD dwFlags = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    
    if (secureProtocol == "TLSv1") {
        dwFlags = WINHTTP_FLAG_SECURE_PROTOCOL_SSL3 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1;
        if (debugMode) std::cout << "Using TLSv1\n";
    } else if (secureProtocol == "TLSv1_1") {
        dwFlags = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1;
        if (debugMode) std::cout << "Using TLSv1.1\n";
    } else if (secureProtocol == "TLSv1_2") {
        dwFlags = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
        if (debugMode) std::cout << "Using TLSv1.2\n";
    } else if (secureProtocol == "TLSv1_3") {
        std::cerr << "Error: TLSv1.3 is not supported by the WinHTTP API on Windows.\n";
        exit(1);
    }

    if (!WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &dwFlags, sizeof(dwFlags))) {
        std::cerr << "Error: Unable to enable specified TLS/SSL protocols.\n";
    } else if (debugMode) {
        std::cout << "Enabled modern TLS/SSL protocols.\n";
    }
}

// Function to log SSL handshake details
void LogSSLHandshake(HINTERNET hRequest) {
    DWORD sslProtocol = 0;
    DWORD size = sizeof(sslProtocol);
    if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &sslProtocol, &size)) {
        std::cout << "SSL/TLS Protocol Used: ";
        if (sslProtocol & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1) {
            std::cout << "TLSv1.0\n";
        } else if (sslProtocol & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1) {
            std::cout << "TLSv1.1\n";
        } else if (sslProtocol & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2) {
            std::cout << "TLSv1.2\n";
        } else {
            std::cout << "Unknown\n";
        }
    }
}

// Function to send an HTTP/HTTPS request
HINTERNET SendRequest(HINTERNET hConnect, const std::wstring& urlPath, bool isSecure) {
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.c_str(),
                                            NULL, WINHTTP_NO_REFERER, 
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            isSecure ? WINHTTP_FLAG_SECURE : 0);

    if (!httpUser.empty() && !httpPassword.empty()) {
        AddBasicAuthHeader(hRequest, httpUser, httpPassword);
    }

    if (ignoreSSLCerts && isSecure) {
        DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | 
                              SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | 
                              SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
    }

    if (debugMode) LogSSLHandshake(hRequest);

    return hRequest;
}

// Helper function to convert std::string to std::wstring
std::wstring string_to_wstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "-V" || arg == "--version") {
            ShowVersion();
            return 0;
        } else if (arg == "-h" || arg == "--help") {
            ShowHelp();
            return 0;
        }
    }

    if (argc < 2) {
        std::cerr << "Usage: s_wget [options] <URL>\n";
        return 1;
    }

    std::wstring url;
    std::wstring outputFileName;
    bool outputFileNameProvided = false;

    // Parse flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-nc" || arg == "--no-clobber") {
            noClobber = true;
        } else if (arg == "--no-check-certificate") {
            ignoreSSLCerts = true;
        } else if (arg == "-t" || arg == "--tries") {
            if (i + 1 < argc) {
                tries = std::stoi(argv[++i]);
                if (tries == 0) {
                    unlimitedRetries = true;
                }
            }
        } else if (arg == "-O" || arg == "--output-document") {
            if (i + 1 < argc) {
                outputFileName = string_to_wstring(argv[++i]);
                outputFileNameProvided = true;
            }
        } else if (arg == "--http-user") {
            if (i + 1 < argc) {
                httpUser = argv[++i];
            }
        } else if (arg == "--http-password") {
            if (i + 1 < argc) {
                httpPassword = argv[++i];
            }
        } else if (arg == "--ftp-user") {
            if (i + 1 < argc) {
                ftpUser = argv[++i];
            }
        } else if (arg == "--ftp-password") {
            if (i + 1 < argc) {
                ftpPassword = argv[++i];
            }
        } else if (arg == "--https-only") {
            httpsOnly = true;
        } else if (arg == "-S" || arg == "--server-response") {
            serverResponse = true;
        } else if (arg == "-d" || arg == "--debug") {
            debugMode = true;
        } else if (arg == "--secure-protocol") {
            if (i + 1 < argc) {
                secureProtocol = argv[++i];
            }
        } else {
            if (url.empty()) {
                url = string_to_wstring(argv[i]);
            } else {
                std::cerr << "Error: Multiple URLs provided.\n";
                return 1;
            }
        }
    }

    if (url.empty()) {
        std::cerr << "Error: No URL provided.\n";
        return 1;
    }

    if (!outputFileNameProvided) {
        outputFileName = GetFileNameFromURL(url);
    }

    if (noClobber && FileExists(outputFileName)) {
        std::wcout << L"File " << outputFileName << L" already exists. Skipping download.\n";
        return 0;
    }

    // Initialize WinHTTP session
    HINTERNET hSession = WinHttpOpen(L"SimpleWget/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        std::cerr << "Error: Unable to open HTTP session.\n";
        return 1;
    }

    EnableTLS(hSession);

    // Handle HTTP/HTTPS Download
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = { 0 }, urlPath[1024] = { 0 };
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(hostName[0]);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(urlPath[0]);

    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp)) {
        std::cerr << "Error: Invalid URL.\n";
        WinHttpCloseHandle(hSession);
        return 1;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect) {
        std::cerr << "Error: Unable to connect.\n";
        WinHttpCloseHandle(hSession);
        return 1;
    }

    bool isSecure = urlComp.nScheme == INTERNET_SCHEME_HTTPS;
    HINTERNET hRequest = SendRequest(hConnect, urlComp.lpszUrlPath, isSecure);

    if (!hRequest) {
        std::cerr << "Error: Unable to create HTTP request.\n";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        std::cerr << "Error: Unable to send request.\n";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        std::cerr << "Error: Unable to receive response.\n";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    if (serverResponse) {
        PrintServerResponse(hRequest);
    }

    std::ofstream outFile(outputFileName, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open output file.\n";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    char buffer[4096];

    do {
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            std::cerr << "Error: Unable to query data availability.\n";
            break;
        }

        if (bytesAvailable == 0)
            break;

        if (bytesAvailable > sizeof(buffer))
            bytesAvailable = sizeof(buffer);

        if (!WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {
            std::cerr << "Error: Unable to read data.\n";
            break;
        }

        if (bytesRead == 0)
            break;

        outFile.write(buffer, bytesRead);
    } while (bytesRead > 0);

    outFile.close();
    std::cout << "Download completed.\n";

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return 0;
}
