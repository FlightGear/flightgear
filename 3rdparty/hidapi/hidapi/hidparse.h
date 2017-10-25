
#ifndef HIDPARSE_H__
#define HIDPARSE_H__

#include <stdint.h>

#include "hidapi.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct hid_item_s {

        /**
         * @brief next our sibling items
         */
        struct hid_item_s *next;

        /**
         * @brief collection our child items
         */
        struct hid_item_s *collection;

        /**
         * @brief parent our parent collection
         */
        struct hid_item_s *parent;

        uint32_t usage;
        uint8_t type;

        uint8_t report_size; /* in bits */
        uint8_t report_id;
        uint8_t flags;

        int32_t logical_min, logical_max, physical_min, physical_max;
        uint32_t unit, unit_exponent;
    } hid_item;

int HID_API_EXPORT HID_API_CALL hid_parse_reportdesc(uint8_t* rdesc_buf, uint32_t rdesc_size, hid_item** root);

void HID_API_EXPORT HID_API_CALL hid_free_reportdesc(hid_item* root);
    
int HID_API_EXPORT HID_API_CALL hid_parse_is_relative(hid_item* item);

#ifdef __cplusplus
}
#endif

#endif
