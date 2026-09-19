#pragma once

#include <esp_ota_ops.h>

// ===================================================================
// Navrat do launcheru: aplikace nahrana z SD karty bezi v oddilu
// ota_1, launcher sedi v ota_0. Hned po startu prepneme bootovaci
// oddil zpet na launcher - jakykoli dalsi restart tedy skonci
// v seznamu aplikaci, aniz by o tom aplikace musela vedet vic.
// Vola se z hwInit().
// ===================================================================

static void bootReturnToLauncher() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *launcher = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
  if (!launcher || running == launcher) return;   // launcher sam nic neprepina
  esp_ota_set_boot_partition(launcher);
}
