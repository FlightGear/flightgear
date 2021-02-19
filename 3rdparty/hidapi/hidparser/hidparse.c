#include "hidparse.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

static uint8_t size_to_bytes(uint8_t sz)
{
    switch (sz)
    {
    case 0: return 0;
    case 1: return 1;
    case 2: return 2;
    case 3: return 4;
    default: return 0;
    }
}

static int32_t read_int(uint8_t sz, uint8_t* raw)
{
    switch (sz)
    {
    case 1: return *((int8_t*) raw); // ensure we cast to sign-extend
    case 2: return *((int16_t*) raw);
    case 3: return *((int32_t*) raw);
    }

    assert(0);
    return 0;
}

static uint32_t read_uint(uint8_t sz, uint8_t* raw)
{
    switch (sz)
    {
    case 1: return *raw;
    case 2: return *((uint16_t*) raw);
    case 3: return *((uint32_t*) raw);
    }

    assert(0);
    return 0;
}


const uint8_t HID_LONG_ITEM = 0xFE; // see 6.2.2.3

// following definitions come from 6.2.2.4
#define HID_COLLECTION_ITEM 0xA
#define HID_END_COLLECTION_ITEM 0xC
#define HID_INPUT_ITEM 0x8
#define HID_OUTPUT_ITEM 0x9
#define HID_FEATURE_ITEM 0xB

#define HID_USAGE_PAGE_ITEM 0x0
#define HID_LOGICAL_MINIMUM_ITEM 0x1
#define HID_LOGICAL_MAXIMUM_ITEM 0x2
#define HID_PHYSICAL_MINIMUM_ITEM 0x3
#define HID_PHYSICAL_MAXIMUM_ITEM 0x4
#define HID_EXPONENT_ITEM 0x5
#define HID_UNIT_ITEM 0x6
#define HID_REPORT_SIZE_ITEM 0x7
#define HID_REPORT_ID_ITEM 0x8
#define HID_REPORT_COUNT_ITEM 0x9
#define HID_PUSH_ITEM 0xA
#define HID_POP_ITEM 0xB

// following definitions come from 6.2.2.8
#define HID_USAGE_ITEM 0x0
#define HID_USAGE_MINIMUM_ITEM 0x1
#define HID_USAGE_MAXIMUM_ITEM 0x2
#define HID_DESIGNATOR_INDEX_ITEM 0x3
#define HID_DESIGNATOR_MINIMUM_ITEM 0x4
#define HID_DESIGNATOR_MAXIMUM_ITEM 0x5
#define HID_STRING_INDEX_ITEM 0x7
#define HID_STRING_MINIMUM_ITEM 0x8
#define HID_STRING_MAXIMUM_ITEM 0x9
#define HID_DELIMITER_ITEM 0xA

#define MAX_ITEM_STACK 4

hid_item item_stack[MAX_ITEM_STACK];
uint8_t item_stack_size = 0;

uint32_t usage_array[16];
uint8_t usage_count;

void init_local_data();
void clear_local_data();

void set_local_minimum(uint8_t tag, uint32_t data);
void set_local_maximum(uint8_t tag, uint32_t data);

void push_local_item(uint8_t tag, uint32_t data);

uint32_t pop_local_item(uint8_t tag);

hid_item* build_item(uint8_t tag, uint8_t flags, hid_item* global_state)
{
    hid_item* item = (hid_item*) calloc(sizeof(hid_item), 1);
    if (item == NULL)
        return NULL;

    item->flags = flags;
    item->usage = pop_local_item(HID_USAGE_ITEM);
    item->type = tag;

// copy from global state
    item->report_size = global_state->report_size;
    item->logical_min = global_state->logical_min;
    item->logical_max = global_state->logical_max;
    item->physical_min = global_state->physical_min;
    item->physical_max = global_state->physical_max;
    item->unit = global_state->unit;
    item->unit_exponent = global_state->unit_exponent;
    item->report_id = global_state->report_id;

    return item;
}

hid_item* build_collection(uint8_t collectionType)
{
    hid_item* col = (hid_item*) calloc(sizeof(hid_item), 1);
    assert(col);
    col->type = collectionType;
    col->usage = pop_local_item(HID_USAGE_ITEM);
    return col;
}

void append_item_with_head(hid_item* head, hid_item* i)
{
    assert(head);
    for (; head->next; head = head->next) {}
    head->next = i;
}

void append_item(hid_item* col, hid_item* i)
{
    assert(col);
    assert(i);
    assert(i->next == NULL);
    assert(i->parent == NULL);

    i->parent = col;
    hid_item* last = col->collection;
    if (!last)
    {
        col->collection = i;
        return;
    }

    append_item_with_head(last, i);
}

int hid_parse_reportdesc(uint8_t* rdesc_buf, uint32_t rdesc_size, hid_item** item)
{
    hid_item current_state;
    uint8_t report_id   = 0;
    uint8_t report_count = 1;
    uint8_t *p = rdesc_buf;
    uint8_t* pEnd = p + rdesc_size;

    hid_item* root_collection = NULL;
    hid_item* current_collection = NULL;

    // zero the entire array
    memset(item_stack, 0, sizeof(hid_item) * MAX_ITEM_STACK);
    item_stack_size = 0;

    memset(&current_state, 0, sizeof(hid_item));
    usage_count = 0;

    init_local_data();

    while (p < pEnd)
    {
         /* See 6.2.2.2 Short Items */
         uint8_t pfx     = *p++;
         if (pfx == HID_LONG_ITEM)
         {
             printf("encountered long item\n");
         }

         uint8_t size    = pfx & 0x3; /* bits 0-1 */
         uint8_t bytes   = size_to_bytes(size);
         uint8_t type    = (pfx >> 2) & 0x3; /* bits 3-2 */
         uint8_t tag     = pfx >> 4;

     //    fprintf(stderr, "short item: size=%d, bytes=%d, type = %d, tag=%d\n",
     //           size, bytes, type, tag);

         /* If it's a main item */
         if (type == 0)
         {
             switch (tag)
             {
             case HID_COLLECTION_ITEM:
             {
          //       fprintf(stderr, "opening collection\n");
                 uint8_t flags = read_uint(size, p);
                 hid_item* col = build_collection(flags);
                 if (!current_collection) {
                     if (root_collection) {
                         append_item_with_head(root_collection, col);
                     } else {
                         root_collection = col;
                     }
                    current_collection = col;
                 } else {
                     append_item(current_collection, col);
                     current_collection = col;
                 }
                 break;
            }

             case HID_END_COLLECTION_ITEM:
              //   fprintf(stderr, "closing collection\n");
                 assert(current_collection);
                 current_collection = current_collection->parent;
                 break;

             case HID_INPUT_ITEM:
             case HID_OUTPUT_ITEM:
             case HID_FEATURE_ITEM:
             {
                 int i;
                 uint8_t flags = read_uint(size, p);

                 for (i=0; i<report_count; ++i) {
                     hid_item* item = build_item(tag, flags, &current_state);
                //     fprintf(stderr, "adding: %d report:%d bits:%d\n", item->type,
                 //    item->report_id, item->report_size);
                     append_item(current_collection, item);
                 }
                 break;
             }

             default:
                 fprintf(stderr, "bad item %d\n", tag);
             }
             /* reset all local data */
             clear_local_data();
         }
         /* Else, if it's a global item */
         else if (type == 1)
         {
             switch (tag)
             {
             case HID_USAGE_PAGE_ITEM:
                 current_state.usage = read_uint(size, p) << 16;
                 break;

             case HID_LOGICAL_MINIMUM_ITEM:
                 current_state.logical_min = read_int(size, p);
                 break;

             case HID_LOGICAL_MAXIMUM_ITEM:
                 current_state.logical_max = read_int(size, p);
                 break;

             case HID_UNIT_ITEM:
                 current_state.unit = read_uint(size, p);
                 break;

             case HID_EXPONENT_ITEM:
                 current_state.unit_exponent = read_uint(size, p);
                 break;

            case HID_REPORT_ID_ITEM:
                report_id = read_uint(size, p);
                //fprintf(stderr, "report id is:%d\n", report_id);
                current_state.report_id = report_id;
                break;

            case HID_REPORT_SIZE_ITEM:
                current_state.report_size = read_uint(size, p);
                //fprintf(stderr, "item size is:%d\n", current_state.report_size);
                break;

            case HID_REPORT_COUNT_ITEM:
                report_count = read_uint(size, p);
                //fprintf(stderr, "item count is:%d\n", report_count);
                break;

            case HID_POP_ITEM:
                assert(item_stack_size > 0);
                current_state = item_stack[--item_stack_size];
                break;

            case HID_PUSH_ITEM:
                item_stack[item_stack_size++] = current_state;
                break;
             }
         } else if (type == 2) {
             /* it's a local item */
             uint32_t value = read_uint(size, p);
             switch (tag)
             {
             /* section 6.2.2.8, remark about Usage size; current_state only
                ever holds a usage page
            */
            case HID_USAGE_ITEM:
                if (size < sizeof(uint32_t))
                    value |= current_state.usage;
                push_local_item(tag, value);
                break;

            case HID_USAGE_MINIMUM_ITEM:
                set_local_minimum(HID_USAGE_ITEM, value | current_state.usage);
                break;

            case HID_USAGE_MAXIMUM_ITEM:
                set_local_maximum(HID_USAGE_ITEM, value | current_state.usage);
                break;

             case HID_DESIGNATOR_MINIMUM_ITEM:
                 set_local_minimum(HID_DESIGNATOR_INDEX_ITEM, value);
                 break;

             case HID_DESIGNATOR_MAXIMUM_ITEM:
                 set_local_maximum(HID_DESIGNATOR_INDEX_ITEM, value);
                  break;

              case HID_STRING_MINIMUM_ITEM:
                  set_local_minimum(HID_STRING_INDEX_ITEM, value);
                  break;

              case HID_STRING_MAXIMUM_ITEM:
                  set_local_maximum(HID_STRING_INDEX_ITEM, value);
                   break;

             default:
                 push_local_item(tag, value);
             }
         } else {
             fprintf(stderr, "bad type value: %d\n", type);
         }

         p += bytes;
    }

    clear_local_data(); // free transient parsing data
    *item = root_collection;
    return 0;
}

void hid_free_reportdesc(hid_item* root)
{
    free(root);
}

int hid_parse_is_relative(hid_item* item)
{
    return item->flags & 0x04; // bit 2 according to HID 6.2.2.5
}


typedef struct {
    uint8_t allocated, count;
    uint32_t* d;
    uint32_t minimum, maximum, step;
} local_data_array;

local_data_array local_data_store[0xf];

void init_local_data()
{
    int i;
    for (i=0; i<0xf; ++i) {
        memset(&local_data_store[i], 0, sizeof(local_data_array));
    }
}

void clear_local_data()
{
    int i;
    for (i=0; i<0xf; ++i) {
        if (local_data_store[i].d != NULL) {
            free(local_data_store[i].d);
        }

        memset(&local_data_store[i], 0, sizeof(local_data_array));
    }
}

void reallocate_local_item(uint8_t tag)
{
    local_data_array* arr = &local_data_store[tag >> 4];
    arr->allocated = (arr->allocated == 0) ? 4 : arr->allocated << 1;
    arr->d = realloc(arr->d, sizeof(uint32_t) * arr->allocated);
}

void push_local_item(uint8_t tag, uint32_t data)
{
    local_data_array* arr = &local_data_store[tag >> 4];
    if (arr->allocated == arr->count)
        reallocate_local_item(tag);

    arr->d[arr->count++] = data;
}

uint32_t pop_local_item(uint8_t tag)
{
    local_data_array* arr = &local_data_store[tag >> 4];
    if (arr->minimum)
    {
        uint32_t result = arr->minimum + arr->step++;
        if (arr->maximum && (result > arr->maximum)) {
            fprintf(stderr, "hidparse.c:pop_local_item: range exceeded for tag %d", tag);
        }
        
        return result;
    }

    if (arr->count == 0)
        return 0;

    uint32_t result = arr->d[arr->count - 1];
    if (arr->count > 1)
        --arr->count; /* pop off the back */
    return result;
}

void set_local_minimum(uint8_t tag, uint32_t data)
{
    local_data_array* arr = &local_data_store[tag >> 4];
    arr->minimum = data;
}

void set_local_maximum(uint8_t tag, uint32_t data)
{
    local_data_array* arr = &local_data_store[tag >> 4];
    arr->maximum = data;
}
