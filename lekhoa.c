#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <time.h>

void http_response(int client_socket, int connection_count) {
    char hostname[256];
    struct utsname system_info;
    struct sysinfo sys_info;
    char html_content[4096];
    char uptime_str[64];
    time_t current_time;
    struct tm *time_info;
    char time_str[64];
    
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
        "    <li><strong>Địa chỉ IP:</strong> 192.168.119.120</li>\r\n"
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
