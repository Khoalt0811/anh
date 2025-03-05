/**
 * HTTP Server với thông tin hệ thống
 * Tác giả: Khoalt0811
 * Ngày: 2025-03-05 08:19:17
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <time.h>

#define PORT 8080
#define SERVER_IP "192.168.119.120"
#define BUFFER_SIZE 4096

// Biến toàn cục để đếm số kết nối
int connection_count = 0;

void http_response(int client_socket) {
    char hostname[256];
    struct utsname system_info;
    struct sysinfo sys_info;
    char html_content[BUFFER_SIZE];
    char uptime_str[64];
    time_t current_time;
    struct tm *time_info;
    char time_str[64];
    
    // Tăng số kết nối
    connection_count++;
    
    // Lấy hostname
    gethostname(hostname, sizeof(hostname));
    
    // Lấy thông tin hệ thống
    uname(&system_info);
    
    // Lấy thông tin uptime
    sysinfo(&sys_info);
    
    // Tính thời gian hoạt động
    int uptime_hours = sys_info.uptime / 3600;
    int uptime_minutes = (sys_info.uptime % 3600) / 60;
    int uptime_seconds = sys_info.uptime % 60;
    snprintf(uptime_str, sizeof(uptime_str), "%d giờ, %d phút, %d giây", 
             uptime_hours, uptime_minutes, uptime_seconds);
    
    // Lấy thời gian hiện tại
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
    
    // Tạo nội dung HTML
    snprintf(html_content, sizeof(html_content),
        "<html>\r\n"
        "<head>\r\n"
        "  <title>Thông tin Server</title>\r\n"
        "  <style>\r\n"
        "    body { font-family: Arial, sans-serif; margin: 20px; }\r\n"
        "    h1 { color: #336699; }\r\n"
        "    h2 { color: #6699cc; margin-top: 20px; }\r\n"
        "    ul { list-style-type: none; padding-left: 10px; }\r\n"
        "    li { margin: 8px 0; }\r\n"
        "    .container { border: 1px solid #ddd; padding: 15px; border-radius: 5px; }\r\n"
        "    .footer { margin-top: 30px; font-size: 0.8em; color: #666; }\r\n"
        "  </style>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "<div class='container'>\r\n"
        "  <h1>Thông tin Server</h1>\r\n"
        "  <h2>Thông tin HTTP</h2>\r\n"
        "  <ul>\r\n"
        "    <li><strong>Số kết nối đã xử lý:</strong> %d</li>\r\n"
        "    <li><strong>Thời gian server hoạt động:</strong> %s</li>\r\n"
        "  </ul>\r\n"
        "  <h2>Thông tin hệ thống</h2>\r\n"
        "  <ul>\r\n"
        "    <li><strong>Hostname:</strong> %s</li>\r\n"
        "    <li><strong>Hệ thống:</strong> %s %s</li>\r\n"
        "    <li><strong>Kiến trúc CPU:</strong> %s</li>\r\n"
        "    <li><strong>Kernel:</strong> %s</li>\r\n"
        "    <li><strong>Địa chỉ IP:</strong> %s</li>\r\n"
        "  </ul>\r\n"
        "  <h2>Thông tin người dùng</h2>\r\n"
        "  <ul>\r\n"
        "    <li><strong>Tên người dùng:</strong> Khoalt0811</li>\r\n"
        "    <li><strong>Thời gian hiện tại (UTC):</strong> 2025-03-05 08:19:17</li>\r\n"
        "    <li><strong>Thời gian local:</strong> %s</li>\r\n"
        "  </ul>\r\n"
        "</div>\r\n"
        "<div class='footer'>\r\n"
        "  <p>Server được tạo bởi Khoalt0811 &copy; 2025</p>\r\n"
        "</div>\r\n"
        "</body>\r\n"
        "</html>",
        connection_count, 
        uptime_str, 
        hostname, 
        system_info.sysname, system_info.release, 
        system_info.machine, 
        system_info.version,
        SERVER_IP,
        time_str
    );
    
    // Tạo header HTTP
    char http_header[512];
    snprintf(http_header, sizeof(http_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "Server: KhoalT0811-HTTPServer/1.0\r\n"
        "\r\n",
        strlen(html_content)
    );
    
    // Gửi header
    write(client_socket, http_header, strlen(http_header));
    
    // Gửi nội dung
    write(client_socket, html_content, strlen(html_content));
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Tạo socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Thiết lập socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Thiết lập địa chỉ server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP);
    address.sin_port = htons(PORT);
    
    // Bind socket vào địa chỉ và port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen cho kết nối đến
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("HTTP Server đang chạy tại http://%s:%d\n", SERVER_IP, PORT);
    printf("Người dùng: Khoalt0811\n");
    printf("Thời gian hiện tại (UTC): 2025-03-05 08:19:17\n");
    printf("Nhấn Ctrl+C để dừng server.\n");
    
    // Vòng lặp xử lý các kết nối
    while(1) {
        printf("Đang đợi kết nối...\n");
        
        // Chấp nhận kết nối đến
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("Kết nối từ %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        
        // Xử lý kết nối
        char buffer[BUFFER_SIZE] = {0};
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            printf("Request nhận được:\n%.100s...\n", buffer);
        }
        
        // Gửi response
        http_response(client_socket);
        
        // Đóng kết nối
        close(client_socket);
        printf("Đóng kết nối. Tổng số kết nối đã xử lý: %d\n", connection_count);
    }
    
    // Đóng server socket
    close(server_fd);
    return 0;
}
