#include <core2.h>
#include <ESPTelnet.h>
#include <EscapeCodes.h>
#include <getopt.h>

// https://github.com/LennartHennigs/ESPTelnet
// MIT License

#define TELNET_PORT 1123

ESPTelnet telnet;
EscapeCodes ansi;

core2_shell_cmd_t shell_commands[64];

#define tprintfln(...)              \
    do                              \
    {                               \
        telnet.printf(__VA_ARGS__); \
        telnet.print("\r\n");       \
    } while (false)

void print_prompt()
{
    telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
    telnet.print("core$");
    telnet.print(ansi.reset());
    telnet.print(" ");
}

void core2_shell_register(const char *func_name, core2_shell_func func)
{
    for (size_t i = 0; i < (sizeof(shell_commands) / sizeof(*shell_commands)); i++)
    {
        if (shell_commands[i].name == NULL)
        {
            shell_commands[i].name = func_name; // TODO: Copy string?
            shell_commands[i].func = func;
            return;
        }

        if (shell_commands[i].name != NULL && !strcmp(shell_commands[i].name, func_name))
        {
            dprintf("core2_shell_register() fail, function already exists '%s'\n", func_name);
            return;
        }
    }
}

void core2_shellcmd_help()
{
    for (size_t i = 0; i < (sizeof(shell_commands) / sizeof(*shell_commands)); i++)
    {
        if (shell_commands[i].name == NULL)
            return;

        tprintfln(" %s @ %p", shell_commands[i].name, (void *)shell_commands[i].func);
    }
}

void core2_shell_init_commands()
{
    for (size_t i = 0; i < (sizeof(shell_commands) / sizeof(*shell_commands)); i++)
    {
        shell_commands[i] = {0};
    }

    core2_shell_register("help", core2_shellcmd_help);
}

bool core2_shell_invoke(String full_command)
{
    const char *func_name = full_command.c_str(); // TODO: Parse properly



    for (size_t i = 0; i < (sizeof(shell_commands) / sizeof(*shell_commands)); i++)
    {
        if (shell_commands[i].name != NULL && !strcmp(shell_commands[i].name, func_name))
        {
            dprintf(">> Executing '%s'\n", func_name);

            shell_commands[i].func();
            return true;
        }
    }

    return false;
}

// ================================================================================

void onTelnetConnect(String ip)
{
    dprintf("# Telnet: %s connected\n", ip.c_str());

    tprintfln("Welcome %s, 'quit' to disconnect", telnet.getIP().c_str());
    tprintfln("");
    print_prompt();
}

void onTelnetDisconnect(String ip)
{
    dprintf("# Telnet: %s disconnected\n", ip.c_str());
}

void onTelnetReconnect(String ip)
{
    dprintf("# Telnet: %s reconnected\n", ip.c_str());

    tprintfln("Welcome back");
    tprintfln("");
    print_prompt();
}

void onTelnetConnectionAttempt(String ip)
{
    dprintf("# Telnet: %s tried to connected\n", ip.c_str());
}

void onTelnetInput(String str)
{
    if (str == "quit")
    {
        tprintfln("Leaving shell prompt");
        telnet.disconnectClient();
        return;
    }

    if (!core2_shell_invoke(str))
    {
        tprintfln("Unknown command '%s'", str.c_str());
    }

    tprintfln("");
    print_prompt();
}

// ================================================================================

void c2_telnet_task(void *params)
{
    for (;;)
    {
        telnet.loop();
        // vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

void core2_shell_init()
{
    dprintf("core2_shell_init()\n");
    core2_shell_init_commands();

    telnet.onConnect(onTelnetConnect);
    telnet.onConnectionAttempt(onTelnetConnectionAttempt);
    telnet.onReconnect(onTelnetReconnect);
    telnet.onDisconnect(onTelnetDisconnect);
    telnet.onInputReceived(onTelnetInput);

    if (telnet.begin(TELNET_PORT, false))
    {
        dprintf("Telnet server running on port %d\n", TELNET_PORT);
    }

    xTaskCreate(c2_telnet_task, "c2_telnet_task", 4096, NULL, 1, NULL);
}