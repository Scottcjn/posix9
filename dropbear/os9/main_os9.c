/*
 * main_os9.c - Dropbear SSH Server entry point for Mac OS 9
 *
 * This file provides the Mac OS 9 specific startup and event handling
 * for the Dropbear SSH server.
 */

#include <MacTypes.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Events.h>
#include <AppleEvents.h>

#include "os9_platform.h"

/* Forward declaration of Dropbear main */
extern int dropbear_main(int argc, char *argv[]);

/* ============================================================
 * Mac Toolbox Initialization
 * ============================================================ */

static void InitMacToolbox(void)
{
    /* Initialize QuickDraw */
    InitGraf(&qd.thePort);

    /* Initialize other managers */
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();

    /* Flush events */
    FlushEvents(everyEvent, 0);
}

/* ============================================================
 * Menu Handling
 * ============================================================ */

enum {
    kAppleMenuID = 128,
    kFileMenuID = 129
};

enum {
    kAboutItem = 1,

    kQuitItem = 1
};

static MenuHandle gAppleMenu;
static MenuHandle gFileMenu;

static void SetupMenus(void)
{
    Handle menuBar;

    /* Create menu bar */
    gAppleMenu = NewMenu(kAppleMenuID, "\p\024");  /* Apple symbol */
    AppendMenu(gAppleMenu, "\pAbout POSIX9 SSH...");
    AppendMenu(gAppleMenu, "\p(-");
    AppendResMenu(gAppleMenu, 'DRVR');
    InsertMenu(gAppleMenu, 0);

    gFileMenu = NewMenu(kFileMenuID, "\pFile");
    AppendMenu(gFileMenu, "\pQuit/Q");
    InsertMenu(gFileMenu, 0);

    DrawMenuBar();
}

static void DoMenuCommand(long menuResult)
{
    short menuID = HiWord(menuResult);
    short menuItem = LoWord(menuResult);

    switch (menuID) {
        case kAppleMenuID:
            if (menuItem == kAboutItem) {
                /* Show about dialog */
                Alert(128, NULL);
            } else {
                /* Handle desk accessories */
                Str255 name;
                GetMenuItemText(gAppleMenu, menuItem, name);
                OpenDeskAcc(name);
            }
            break;

        case kFileMenuID:
            if (menuItem == kQuitItem) {
                /* Signal to quit */
                raise(SIGTERM);
            }
            break;
    }

    HiliteMenu(0);
}

/* ============================================================
 * Event Loop Integration
 * ============================================================ */

/* Flag to indicate server should keep running */
static volatile Boolean gRunning = true;

/* Signal handler for graceful shutdown */
static void shutdown_handler(int sig)
{
    (void)sig;
    gRunning = false;
}

/*
 * Process Mac events while server is running.
 * Called periodically from Dropbear's main loop.
 */
void os9_process_events(void)
{
    EventRecord event;

    /* Process signals */
    posix9_signal_process();

    /* Check for Mac events (non-blocking) */
    if (WaitNextEvent(everyEvent, &event, 1, NULL)) {
        switch (event.what) {
            case mouseDown:
                {
                    WindowPtr window;
                    short part = FindWindow(event.where, &window);

                    switch (part) {
                        case inMenuBar:
                            DoMenuCommand(MenuSelect(event.where));
                            break;

                        case inSysWindow:
                            SystemClick(&event, window);
                            break;

                        case inDrag:
                            DragWindow(window, event.where, &qd.screenBits.bounds);
                            break;

                        case inGoAway:
                            if (TrackGoAway(window, event.where)) {
                                /* Hide status window */
                            }
                            break;
                    }
                }
                break;

            case keyDown:
            case autoKey:
                if (event.modifiers & cmdKey) {
                    DoMenuCommand(MenuKey(event.message & charCodeMask));
                }
                break;

            case updateEvt:
                BeginUpdate((WindowPtr)event.message);
                EndUpdate((WindowPtr)event.message);
                break;

            case kHighLevelEvent:
                AEProcessAppleEvent(&event);
                break;
        }
    }

    /* Give time to other processes */
    SystemTask();
}

/*
 * Check if server should continue running.
 */
int os9_should_continue(void)
{
    return gRunning ? 1 : 0;
}

/* ============================================================
 * Status Window
 * ============================================================ */

static WindowPtr gStatusWindow = NULL;

static void CreateStatusWindow(void)
{
    Rect bounds = { 50, 50, 150, 400 };

    gStatusWindow = NewWindow(NULL, &bounds, "\pPOSIX9 SSH Server",
                              true, documentProc, (WindowPtr)-1, true, 0);

    if (gStatusWindow) {
        SetPort(gStatusWindow);
        TextFont(monaco);
        TextSize(9);
        MoveTo(10, 20);
        DrawString("\pSSH Server Running on port 22");
        MoveTo(10, 35);
        DrawString("\pPress Cmd+Q to quit");
    }
}

static void UpdateStatus(const char *message)
{
    Str255 pstr;

    if (!gStatusWindow) return;

    SetPort(gStatusWindow);

    /* Clear and redraw */
    EraseRect(&gStatusWindow->portRect);
    MoveTo(10, 20);
    DrawString("\pSSH Server Running on port 22");
    MoveTo(10, 35);

    /* Convert C string to Pascal string */
    {
        size_t len = strlen(message);
        if (len > 255) len = 255;
        pstr[0] = len;
        memcpy(&pstr[1], message, len);
    }

    DrawString(pstr);
}

/* ============================================================
 * Apple Event Handlers
 * ============================================================ */

static pascal OSErr HandleQuitAE(const AppleEvent *event, AppleEvent *reply, long refcon)
{
    (void)event;
    (void)reply;
    (void)refcon;

    gRunning = false;
    return noErr;
}

static void InstallAppleEventHandlers(void)
{
    AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                          NewAEEventHandlerUPP(HandleQuitAE), 0, false);
}

/* ============================================================
 * Main Entry Point
 * ============================================================ */

int main(void)
{
    int result;
    char *argv[] = {
        "dropbear",
        "-F",           /* Foreground (no fork) */
        "-E",           /* Log to stderr */
        "-p", "22",     /* Port 22 */
        NULL
    };
    int argc = 5;

    /* Initialize Mac Toolbox */
    InitMacToolbox();

    /* Set up menus */
    SetupMenus();

    /* Install Apple Event handlers */
    InstallAppleEventHandlers();

    /* Create status window */
    CreateStatusWindow();

    /* Initialize POSIX9 platform */
    if (os9_platform_init() != 0) {
        Alert(129, NULL);  /* Error alert */
        return 1;
    }

    /* Install signal handlers */
    signal(SIGTERM, shutdown_handler);
    signal(SIGINT, shutdown_handler);

    /* Log startup */
    syslog(LOG_INFO, "POSIX9 SSH Server starting on port 22");
    UpdateStatus("Starting...");

    /* Run Dropbear - this function contains the main server loop */
    result = dropbear_main(argc, argv);

    /* Cleanup */
    syslog(LOG_INFO, "POSIX9 SSH Server shutting down");
    os9_platform_cleanup();

    if (gStatusWindow) {
        DisposeWindow(gStatusWindow);
    }

    return result;
}

/* ============================================================
 * Dropbear Integration Hooks
 * ============================================================ */

/*
 * Called by Dropbear's main loop to process OS events.
 * This keeps the Mac responsive while waiting for connections.
 */
void dropbear_os9_idle(void)
{
    os9_process_events();
}

/*
 * Called when a new connection is accepted.
 */
void dropbear_os9_connection(const char *client_ip)
{
    char msg[128];
    sprintf(msg, "Connected: %s", client_ip);
    UpdateStatus(msg);
    syslog(LOG_INFO, "Connection from %s", client_ip);
}

/*
 * Called when a connection is closed.
 */
void dropbear_os9_disconnect(const char *client_ip)
{
    syslog(LOG_INFO, "Disconnected: %s", client_ip);
    UpdateStatus("Waiting for connections...");
}
