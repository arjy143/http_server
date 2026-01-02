#define ATTEST_IMPLEMENTATION
#include "attest.h"

#include <string.h>
#include <stdlib.h>

// Include headers from the main project
#include "http.h"
#include "file.h"
#include "config.h"

// ============================================================================
// Tests for parse_request_line
// ============================================================================

REGISTER_TEST(parse_request_line_get_index)
{
    char method[16];
    char path[256];

    parse_request_line("GET /index.html HTTP/1.1\r\n", method, sizeof(method), path, sizeof(path));

    ATTEST_EQUAL(method, "GET");
    ATTEST_EQUAL(path, "/index.html");
}

REGISTER_TEST(parse_request_line_post)
{
    char method[16];
    char path[256];

    parse_request_line("POST /api/data HTTP/1.1\r\n", method, sizeof(method), path, sizeof(path));

    ATTEST_EQUAL(method, "POST");
    ATTEST_EQUAL(path, "/api/data");
}

REGISTER_TEST(parse_request_line_root_path)
{
    char method[16];
    char path[256];

    parse_request_line("GET / HTTP/1.1\r\n", method, sizeof(method), path, sizeof(path));

    ATTEST_EQUAL(method, "GET");
    ATTEST_EQUAL(path, "/");
}

REGISTER_TEST(parse_request_line_with_query)
{
    char method[16];
    char path[256];

    parse_request_line("GET /search?q=test HTTP/1.1\r\n", method, sizeof(method), path, sizeof(path));

    ATTEST_EQUAL(method, "GET");
    ATTEST_EQUAL(path, "/search?q=test");
}

REGISTER_TEST(parse_request_line_empty_returns_empty)
{
    char method[16] = "initial";
    char path[256] = "initial";

    // No \r\n means invalid request line
    parse_request_line("", method, sizeof(method), path, sizeof(path));

    ATTEST_EQUAL(method, "");
    ATTEST_EQUAL(path, "");
}

REGISTER_TEST(parse_request_line_nested_path)
{
    char method[16];
    char path[256];

    parse_request_line("GET /path/to/file.css HTTP/1.1\r\n", method, sizeof(method), path, sizeof(path));

    ATTEST_EQUAL(method, "GET");
    ATTEST_EQUAL(path, "/path/to/file.css");
}

// ============================================================================
// Tests for sanitise_path
// ============================================================================

REGISTER_TEST(sanitise_path_normal_path)
{
    ATTEST_EQUAL(sanitise_path("/index.html"), 1);
}

REGISTER_TEST(sanitise_path_nested_path)
{
    ATTEST_EQUAL(sanitise_path("/css/style.css"), 1);
}

REGISTER_TEST(sanitise_path_blocks_dotdot)
{
    ATTEST_EQUAL(sanitise_path("/../etc/passwd"), 0);
}

REGISTER_TEST(sanitise_path_blocks_dotdot_middle)
{
    ATTEST_EQUAL(sanitise_path("/path/../secret"), 0);
}

REGISTER_TEST(sanitise_path_blocks_dotdot_end)
{
    ATTEST_EQUAL(sanitise_path("/path/to/.."), 0);
}

REGISTER_TEST(sanitise_path_allows_single_dot)
{
    ATTEST_EQUAL(sanitise_path("/./file.txt"), 1);
}

REGISTER_TEST(sanitise_path_root)
{
    ATTEST_EQUAL(sanitise_path("/"), 1);
}

// ============================================================================
// Tests for get_mime_type
// ============================================================================

REGISTER_TEST(mime_type_html)
{
    ATTEST_EQUAL(get_mime_type("/index.html"), "text/html");
}

REGISTER_TEST(mime_type_htm)
{
    ATTEST_EQUAL(get_mime_type("/page.htm"), "text/html");
}

REGISTER_TEST(mime_type_css)
{
    ATTEST_EQUAL(get_mime_type("/style.css"), "text/css");
}

REGISTER_TEST(mime_type_js)
{
    ATTEST_EQUAL(get_mime_type("/app.js"), "application/javascript");
}

REGISTER_TEST(mime_type_png)
{
    ATTEST_EQUAL(get_mime_type("/image.png"), "image/png");
}

REGISTER_TEST(mime_type_jpg)
{
    ATTEST_EQUAL(get_mime_type("/photo.jpg"), "image/jpeg");
}

REGISTER_TEST(mime_type_jpeg)
{
    ATTEST_EQUAL(get_mime_type("/photo.jpeg"), "image/jpeg");
}

REGISTER_TEST(mime_type_gif)
{
    ATTEST_EQUAL(get_mime_type("/animation.gif"), "image/gif");
}

REGISTER_TEST(mime_type_svg)
{
    ATTEST_EQUAL(get_mime_type("/icon.svg"), "image/svg+xml");
}

REGISTER_TEST(mime_type_ico)
{
    ATTEST_EQUAL(get_mime_type("/favicon.ico"), "image/x-icon");
}

REGISTER_TEST(mime_type_txt)
{
    ATTEST_EQUAL(get_mime_type("/readme.txt"), "text/plain");
}

REGISTER_TEST(mime_type_json)
{
    ATTEST_EQUAL(get_mime_type("/data.json"), "application/json");
}

REGISTER_TEST(mime_type_pdf)
{
    ATTEST_EQUAL(get_mime_type("/document.pdf"), "application/pdf");
}

REGISTER_TEST(mime_type_xml)
{
    ATTEST_EQUAL(get_mime_type("/config.xml"), "application/xml");
}

REGISTER_TEST(mime_type_woff)
{
    ATTEST_EQUAL(get_mime_type("/font.woff"), "font/woff");
}

REGISTER_TEST(mime_type_woff2)
{
    ATTEST_EQUAL(get_mime_type("/font.woff2"), "font/woff2");
}

REGISTER_TEST(mime_type_unknown)
{
    ATTEST_EQUAL(get_mime_type("/file.xyz"), "application/octet-stream");
}

REGISTER_TEST(mime_type_no_extension)
{
    ATTEST_EQUAL(get_mime_type("/filename"), "application/octet-stream");
}

// ============================================================================
// Tests for config_init
// ============================================================================

REGISTER_TEST(config_init_defaults)
{
    server_config_t config;
    config_init(&config);

    ATTEST_EQUAL(config.port, DEFAULT_PORT);
    ATTEST_EQUAL(config.num_threads, DEFAULT_THREADS);
    ATTEST_EQUAL(config.queue_size, DEFAULT_QUEUE_SIZE);
    ATTEST_EQUAL(config.doc_root, DEFAULT_DOC_ROOT);
    ATTEST_EQUAL(config.verbose, 0);
    ATTEST_EQUAL(config.show_help, 0);
    ATTEST_EQUAL(config.show_version, 0);
}

// ============================================================================
// Tests for config_parse_args
// ============================================================================

REGISTER_TEST(config_parse_args_port_short)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-p", "3000"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.port, 3000);
}

REGISTER_TEST(config_parse_args_port_long)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--port", "9000"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.port, 9000);
}

REGISTER_TEST(config_parse_args_directory_short)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-d", "/var/www"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.doc_root, "/var/www");
}

REGISTER_TEST(config_parse_args_directory_long)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--directory", "/home/user/public"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.doc_root, "/home/user/public");
}

REGISTER_TEST(config_parse_args_threads_short)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-t", "8"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.num_threads, 8);
}

REGISTER_TEST(config_parse_args_threads_long)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--threads", "16"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.num_threads, 16);
}

REGISTER_TEST(config_parse_args_verbose_short)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-v"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.verbose, 1);
}

REGISTER_TEST(config_parse_args_verbose_long)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--verbose"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.verbose, 1);
}

REGISTER_TEST(config_parse_args_help_short)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-h"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.show_help, 1);
}

REGISTER_TEST(config_parse_args_help_long)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--help"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.show_help, 1);
}

REGISTER_TEST(config_parse_args_version)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--version"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.show_version, 1);
}

REGISTER_TEST(config_parse_args_multiple_options)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-p", "8000", "-d", "/srv/www", "-t", "2", "-v"};
    int argc = 8;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, 0);
    ATTEST_EQUAL(config.port, 8000);
    ATTEST_EQUAL(config.doc_root, "/srv/www");
    ATTEST_EQUAL(config.num_threads, 2);
    ATTEST_EQUAL(config.verbose, 1);
}

REGISTER_TEST(config_parse_args_invalid_port_zero)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-p", "0"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_invalid_port_too_high)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-p", "70000"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_invalid_threads_zero)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-t", "0"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_invalid_threads_too_high)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-t", "100"};
    int argc = 3;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_unknown_option)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "--unknown"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_missing_port_value)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-p"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_missing_directory_value)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-d"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

REGISTER_TEST(config_parse_args_missing_threads_value)
{
    server_config_t config;
    config_init(&config);

    char* argv[] = {"http_server", "-t"};
    int argc = 2;

    int result = config_parse_args(&config, argc, argv);

    ATTEST_EQUAL(result, -1);
}

// ============================================================================
// Tests for is_directory (filesystem test)
// ============================================================================

REGISTER_TEST(is_directory_returns_true_for_dir)
{
    // Current directory should always exist and be a directory
    ATTEST_EQUAL(is_directory("."), 1);
}

REGISTER_TEST(is_directory_returns_false_for_file)
{
    // This test file should exist but not be a directory
    ATTEST_EQUAL(is_directory("tests/test_http_server.c"), 0);
}

REGISTER_TEST(is_directory_returns_false_for_nonexistent)
{
    ATTEST_EQUAL(is_directory("/nonexistent/path/12345"), 0);
}
