#include "server.h"
#include "config.h"

int main(int argc, char* argv[])
{
    server_config_t config;
    config_init(&config);

    if (config_parse_args(&config, argc, argv) != 0)
    {
        return 1;
    }

    if (config.show_help)
    {
        config_print_help(argv[0]);
        return 0;
    }

    if (config.show_version)
    {
        config_print_version();
        return 0;
    }

    run_server(&config);
    return 0;
}
