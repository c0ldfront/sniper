#ifndef SNIPER87_EDITOR_RECORDS_H
#define SNIPER87_EDITOR_RECORDS_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t *records;
    size_t count;
    uint32_t checksum; /* Load-time sum: editing deliberately leaves it unchanged. */
    int dirty;
} SniperEditorRecords;
/* Initialize a zeroed object from bytes, ignoring incomplete trailing records. */
int sniper_editor_records_load(SniperEditorRecords *, const uint8_t *, size_t);
/* 40a3e0: first matching record only; zero tile deletes, absent nonzero appends. */
int sniper_editor_records_place(SniperEditorRecords *, int32_t x, int32_t y, int32_t tile);
/* Little-endian serialization; returns required byte count, writes only if it fits. */
size_t sniper_editor_records_bytes(const SniperEditorRecords *, uint8_t *, size_t);
int sniper_editor_records_save_needed(const SniperEditorRecords *);
void sniper_editor_records_free(SniperEditorRecords *);
#endif
