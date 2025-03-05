/**
 * HTTP Server đầy đủ (không cần thư viện phụ thuộc)
 * Tác giả: Khoalt0811
 * Ngày: 2025-03-05 07:12:37
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define SERVER_ROOT "."  // Thư mục gốc của server

// Biến toàn cục để theo dõi số kết nối
volatile int connection_count = 0;
volatile int server_running = 1;

// Cấu trúc HTTP request
typedef struct {
    char method[16];
    char uri[512];
    char protocol[16];
    char host[128];
    char user_agent[256];
} HttpRequest;

// Hàm để phân tích HTTP request
void parse_request(const char* request_str, HttpRequest* request) {
    // Mặc định
    strcpy(request->method, "");
    strcpy(request->uri, "/");
    strcpy(request->protocol, "");
    strcpy(request->host, "");
    strcpy(request->user_agent, "");
    
    // Phân tích dòng đầu tiên
    char* line = strdup(request_str);
    char* rest = NULL;
    char* token = strtok_r(line, "\r\n", &rest);
    if (token != NULL) {
        sscanf(token, "%15s %511s %15s", request->method, request->uri, request->protocol);
    }
    free(line);
    
    // Phân tích các header
    line = strdup(request_str);
    rest = NULL;
    token = strtok_r(line, "\r\n", &rest);
    
    while ((token = strtok_r(NULL, "\r\n", &rest)) != NULL) {
        if (strncmp(token, "Host: ", 6) == 0) {
            strncpy(request->host, token + 6, sizeof(request->host) - 1);
        }
        else if (strncmp(token, "User-Agent: ", 12) == 0) {
            strncpy(request->user_agent, token + 12, sizeof(request->user_agent) - 1);
        }
    }
    free(line);
}

// Lấy MIME type dựa trên phần mở rộng file
const char* get_mime_type(const char* file_path) {
    const char* dot = strrchr(file_path, '.');
    if (dot) {
        if (strcasecmp(dot, ".html") == 0 || strcasecmp(dot, ".htm") == 0)
            return "text/html";
        if (strcasecmp(dot, ".txt") == 0)
            return "text/plain";
        if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0)
            return "image/jpeg";
        if (strcasecmp(dot, ".png") == 0)
            return "image/png";
        if (strcasecmp(dot, ".gif") == 0)
            return "image/gif";
        if (strcasecmp(dot, ".css") == 0)
            return "text/css";
        if (strcasecmp(dot, ".js") == 0)
            return "application/javascript";
        if (strcasecmp(dot, ".pdf") == 0)
            return "application/pdf";
        if (strcasecmp(dot, ".json") == 0)
            return "application/json";
    }
    return "application/octet-stream";  // Mặc định binary
}

// Tạo HTML cho thư mục
void generate_directory_listing(const char* path, const char* uri, char* buffer, size_t buffer_size) {
    DIR* dir;
    struct dirent* entry;
    
    snprintf(buffer, buffer_size,
        "<html>\r\n"
        "<head><title>Index of %s</title></head>\r\n"
        "<body>\r\n"
        "<h1>Index of %s</h1>\r\n"
        "<table>\r\n"
        "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\r\n"
        "<tr><td><a href=\"..\">..</a></td><td>-</td><td>-</td></tr>\r\n",
        uri, uri);
    
    dir = opendir(path);
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
                
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            
            struct stat file_stat;
            if (stat(full_path, &file_stat) == 0) {
                char modified_time[64];
                strftime(modified_time, sizeof(modified_time), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
                
                char size_str[32];
                if (S_ISDIR(file_stat.st_mode)) {
                    strcpy(size_str, "-");
                } else {
                    if (file_stat.st_size < 1024)
                        snprintf(size_str, sizeof(size_str), "%ld B", file_stat.st_size);
                    else if (file_stat.st_size < 1024*1024)
                        snprintf(size_str, sizeof(size_str), "%.1f KB", file_stat.st_size/1024.0);
                    else
                        snprintf(size_str, sizeof(size_str), "%.1f MB", file_stat.st_size/(1024.0*1024.0));
                }
                
                char link_uri[1024];
                if (strcmp(uri, "/") == 0) {
                    snprintf(link_uri, sizeof(link_uri), "/%s", entry->d_name);
                } else {
                    snprintf(link_uri, sizeof(link_uri), "%s/%s", uri, entry->d_name);
                }
                
                char entry_html[2048];
                snprintf(entry_html, sizeof(entry_html),
                    "<tr><td><a href=\"%s\">%s%s</a></td><td>%s</td><td>%s</td></tr>\r\n",
                    link_uri, entry->d_name, S_ISDIR(file_stat.st_mode) ? "/" : "", size_str, modified_time);
                
                size_t current_len = strlen(buffer);
                if (current_len + strlen(entry_html) < buffer_size - 100) {
                    strcat(buffer, entry_html);
                }
            }
        }
        closedir(dir);
    }
    
    // Thêm thông tin footer
    char footer[1024];
    time_t now = time(NULL);
    struct tm* time_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
    
    snprintf(footer, sizeof(footer),
        "</table>\r\n"
        "<hr>\r\n"
        "<p>HTTP Server của Khoalt0811 - %s</p>\r\n"
        "</body>\r\n"
        "</html>",
        time_str);
    
    strcat(buffer, footer);
}

// Tạo trang lỗi 404
void generate_404_page(const char* uri, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
        "<html>\r\n"
        "<head><title>404 Not Found</title></head>\r\n"
        "<body>\r\n"
        "<h1>404 Not Found</h1>\r\n"
        "<p>Tài nguyên bạn yêu cầu không tồn tại: %s</p>\r\n"
        "<hr>\r\n"
        "<p>HTTP Server của Khoalt0811</p>\r\n"
        "</body>\r\n"
        "</html>",
        uri);
}

// Xử lý tín hiệu
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nNhận tín hiệu Ctrl+C. Đang dừng server...\n");
        server_running = 0;
    }
}

// Hàm chính để xử lý kết nối
void handle_connection(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    // Đọc HTTP request
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        perror("Lỗi khi đọc từ client");
        close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';
    
    // Tăng số kết nối
    connection_count++;
    
    // Phân tích HTTP request
    HttpRequest request;
    parse_request(buffer, &request);
    
    printf("[Kết nối #%d] %s %s %s\n", connection_count, request.method, request.uri, request.protocol);
    
    // Chuẩn bị đường dẫn file
    char file_path[1024];
    strcpy(file_path, SERVER_ROOT);
    
    if (strcmp(request.uri, "/") == 0) {
        // Trang chính
        char html_content[BUFFER_SIZE];
        
        time_t now = time(NULL);
        struct tm* time_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
        
        snprintf(html_content, sizeof(html_content),
            "<html>\r\n"
            "<head><title>HTTP Server bởi Khoalt0811</title></head>\r\n"
            "<body>\r\n"
            "<h1>HTTP Server đang chạy!</h1>\r\n"
            "<p>Đây là server HTTP đơn giản được viết bằng C mà không sử dụng thư viện phụ thuộc.</p>\r\n"
            "<h2>Thông tin server:</h2>\r\n"
            "<ul>\r\n"
            "<li>Thời gian hiện tại: %s</li>\r\n"
            "<li>Người dùng: Khoalt0811</li>\r\n"
            "<li>Số kết nối đã xử lý: %d</li>\r\n"
            "</ul>\r\n"
            "<p>Bạn có thể xem:</p>\r\n"
            "<ul>\r\n"
            "<li><a href=\"/files\">Danh sách file</a></li>\r\n"
            "<li><a href=\"/info\">Thông tin server</a></li>\r\n"
            "</ul>\r\n"
            "<hr>\r\n"
            "<p>HTTP Server của Khoalt0811 - %s</p>\r\n"
            "</body>\r\n"
            "</html>\r\n",
            time_str, connection_count, time_str);
        
        // Gửi HTTP response header
        char http_header[512];
        snprintf(http_header, sizeof(http_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            strlen(html_content));
        
        // Gửi header và nội dung HTML
        send(client_socket, http_header, strlen(http_header), 0);
        send(client_socket, html_content, strlen(html_content), 0);
    }
    else if (strcmp(request.uri, "/info") == 0) {
        // Trang thông tin
        char html_content[BUFFER_SIZE];
        
        time_t now = time(NULL);
        struct tm* time_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
        
        // Lấy thông tin hệ thống
        char hostname[128];
        gethostname(hostname, sizeof(hostname));
        
        FILE* fp = popen("uname -a", "r");
        char sys_info[256] = "Unknown";
        if (fp != NULL) {
            if (fgets(sys_info, sizeof(sys_info), fp) == NULL) {
                strcpy(sys_info, "Unknown");
            }
            pclose(fp);
        }
        
        snprintf(html_content, sizeof(html_content),
            "<html>\r\n"
            "<head><title>Thông tin Server</title></head>\r\n"
            "<body>\r\n"
            "<h1>Thông tin Server</h1>\r\n"
            "<h2>Thông tin HTTP</h2>\r\n"
            "<ul>\r\n"
            "<li>Số kết nối đã xử lý: %d</li>\r\n"
            "<li>Thời gian server hoạt động: từ khởi động đến hiện tại</li>\r\n"
            "</ul>\r\n"
            "<h2>Thông tin hệ thống</h2>\r\n"
            "<ul>\r\n"
            "<li>Hostname: %s</li>\r\n"
            "<li>Hệ thống: %s</li>\r\n"
