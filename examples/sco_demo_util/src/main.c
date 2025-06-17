#include <btstack_audio.h>
#include <btstack_include.h>
#include <btstack_tlv_none.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef ESP_PLATFORM
#include <ble/le_device_db_tlv.h>
#include <btstack_tlv_esp32.h>
#include <classic/btstack_link_key_db.h>
#include <classic/btstack_link_key_db_tlv.h>
#endif

#ifdef WIN32
#include <ble/le_device_db_tlv.h>
#include <btstack_stdin.h>
#include <btstack_stdin_windows.h>
#include <btstack_tlv_windows.h>
#include <classic/btstack_link_key_db_tlv.h>
#include <hci_dump.h>
#include <hci_dump_windows_fs.h>
#endif

extern int btstack_main(int argc, const char *argv[]);

static btstack_packet_callback_registration_t hci_event_callback_registration;
static bd_addr_t local_addr;
static bool shutdown_triggered;

#if defined(WIN32)

#define TLV_DB_PATH_PREFIX	"btstack_"
#define TLV_DB_PATH_POSTFIX ".tlv"
static char tlv_db_path[100];
static const btstack_tlv_t *tlv_impl;
static btstack_tlv_windows_t tlv_context;

static void init_tlv() {
    btstack_strcpy(tlv_db_path, sizeof(tlv_db_path), TLV_DB_PATH_PREFIX);
    btstack_strcat(tlv_db_path, sizeof(tlv_db_path), bd_addr_to_str_with_delimiter(local_addr, '-'));
    btstack_strcat(tlv_db_path, sizeof(tlv_db_path), TLV_DB_PATH_POSTFIX);
    tlv_impl = btstack_tlv_windows_init_instance(&tlv_context, tlv_db_path);
    btstack_tlv_set_instance(tlv_impl, &tlv_context);
#ifdef ENABLE_CLASSIC
    hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(tlv_impl, &tlv_context));
#endif
#ifdef ENABLE_BLE
    le_device_db_tlv_configure(tlv_impl, &tlv_context);
#endif
}

static void deinit_tlv() {
    btstack_tlv_windows_deinit(&tlv_context);
}

#elif defined(ESP_PLATFORM)

static void init_tlv() {
    // setup TLV ESP32 implementation and register with system
    const btstack_tlv_t *btstack_tlv_impl = btstack_tlv_esp32_get_instance();
    btstack_tlv_set_instance(btstack_tlv_impl, NULL);
#ifdef ENABLE_CLASSIC
    // setup Link Key DB using TLV
    const btstack_link_key_db_t *btstack_link_key_db = btstack_link_key_db_tlv_get_instance(btstack_tlv_impl, NULL);
    hci_set_link_key_db(btstack_link_key_db);
#endif
#ifdef ENABLE_BLE
    // setup LE Device DB using TLV
    le_device_db_tlv_configure(btstack_tlv_impl, NULL);
#endif
}

static void deinit_tlv() {}

#else

static void init_tlv() {
#ifdef ENABLE_CLASSIC
    hci_set_link_key_db(btstack_link_key_db_memory_instance());
#endif
#ifdef ENABLE_BLE
    le_device_db_tlv_configure(btstack_tlv_none_init_instance(), NULL);
#endif
}

static void deinit_tlv() {}

#endif

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET)
        return;
    if (hci_event_packet_get_type(packet) != BTSTACK_EVENT_STATE)
        return;
    switch (btstack_event_state_get_state(packet)) {
        case HCI_STATE_WORKING:
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            init_tlv();
            break;
        case HCI_STATE_OFF:
            deinit_tlv();
            if (!shutdown_triggered)
                break;
            // reset stdin
            btstack_stdin_reset();
            log_info("Good bye, see you.\n");
            exit(0);
            break;
        default:
            break;
    }
}

static void trigger_shutdown(void) {
    printf("CTRL-C - SIGINT received, shutting down..\n");
    log_info("sigint_handler: shutting down");
    shutdown_triggered = true;
    hci_power_control(HCI_POWER_OFF);
}

#ifdef ESP_PLATFORM
int app_main(void) {
#else
int main(int argc, const char *argv[]) {
#endif

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

#ifdef WIN32
    // log into file using HCI_DUMP_PACKETLOGGER format
    const char *pklg_path = "hci_dump.pklg";
    hci_dump_windows_fs_open(pklg_path, HCI_DUMP_PACKETLOGGER);
    const hci_dump_t *hci_dump_impl = hci_dump_windows_fs_get_instance();
    hci_dump_init(hci_dump_impl);
    printf("Packet Log: %s\n", pklg_path);

    // setup stdin to handle CTRL-c
    btstack_stdin_windows_init();
    btstack_stdin_window_register_ctrl_c_callback(&trigger_shutdown);
#endif

#ifdef HAVE_PORTAUDIO
    btstack_audio_sink_set_instance(btstack_audio_portaudio_sink_get_instance());
    btstack_audio_source_set_instance(btstack_audio_portaudio_source_get_instance());
#endif

    // Setup example
    btstack_main(0, NULL);

    // Enter run loop (forever)
    btstack_run_loop_execute();

    return 0;
}
