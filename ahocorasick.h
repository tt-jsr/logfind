#pragma once

#include <stdbool.h>
#include "aho_trie.h"
#include "aho_text.h"

struct aho_match_t
{
    int id;                             // id of pattern returned from aho_add_match_text()
    unsigned long long int pos;         // Match at offset into line or file
    unsigned long long int lineno;      // Line number match was found
    unsigned long long int lineoff;     // File offset of line in file

    int len;                            // Length of match
};

struct ahocorasick
{
#define AHO_MAX_TEXT_ID INT_MAX
    int accumulate_text_id;
    struct aho_text_t* text_list_head;
    struct aho_text_t* text_list_tail;
    int text_list_len;

    struct aho_trie trie;

    void (*callback_match)(void* arg, struct aho_match_t*);
    void* callback_arg;
};

void aho_init(struct ahocorasick * aho);
void aho_destroy(struct ahocorasick * aho);

int aho_add_match_text(struct ahocorasick * aho, const char* text, unsigned int len);
bool aho_del_match_text(struct ahocorasick * aho, const int id);
void aho_clear_match_text(struct ahocorasick * aho);

void aho_create_trie(struct ahocorasick * aho);
void aho_clear_trie(struct ahocorasick * aho);

unsigned int aho_findtext(struct ahocorasick * aho, unsigned long long int lineno, unsigned long long int lineoff, char (*getchar)(void *), void *arg);

void aho_register_match_callback(struct ahocorasick * aho,
        void (*callback_match)(void* arg, struct aho_match_t*),
        void *arg);

/* for debug */
void aho_print_match_text(struct ahocorasick * aho);
