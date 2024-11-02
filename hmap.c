// map_lib
// A simple associative-array library for C
//
// License: MIT / X11
// Copyright (c) 2009 by James K. Lawless
// jimbo@radiks.net http://www.radiks.net/~jimbo
// http://www.mailsend-online.com
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NNStrMap NNStrMap;
typedef struct NNBoxedString NNBoxedString;

struct NNBoxedString
{
    bool isalloced;
    char* data;
    size_t length;
};

struct NNStrMap
{
    NNStrMap* nxt;
    NNBoxedString* name;
    char* value;
};

NNBoxedString* nn_strbox_makelen(char* str, size_t len, bool shouldcopy)
{
    size_t allocsz;
    NNBoxedString* cs;
    cs = (NNBoxedString*)malloc(sizeof(NNBoxedString));
    cs->isalloced = shouldcopy;
    if(shouldcopy)
    {
        allocsz = sizeof(char) * (len+1);
        cs->data = (char*)malloc(allocsz);
        memset(cs->data, 0, allocsz);
        memcpy(cs->data, str, len);
    }
    else
    {
        cs->data = str;
    }
    cs->length = len;
    return cs;
}

NNBoxedString* nn_strbox_make(char* str, bool shouldcopy)
{
    return nn_strbox_makelen(str, strlen(str), shouldcopy); 
}

void nn_strbox_destroy(NNBoxedString* cs)
{
    if(cs->isalloced)
    {
        free(cs->data);
    }
    cs->data = NULL;
    cs->length = 0;
    free(cs);
}

NNStrMap* nn_strmap_make()
{
    NNStrMap* map;
    map = (NNStrMap*)malloc(sizeof(NNStrMap));
    map->name = NULL;
    map->value = NULL;
    map->nxt = NULL;
}

void nn_strmap_destroy(NNStrMap* map)
{
    NNStrMap* sub;
    NNStrMap* tmp;
    sub = map;
    while(sub != NULL)
    {
        if(sub->name != NULL)
        {
            nn_strbox_destroy(sub->name);
        }
        if(sub->value != NULL)
        {
            free(sub->value);
            sub->value = NULL;
        }
        tmp = sub->nxt;
        if(sub != map)
        {
            free(sub);
        }
        sub = tmp;
    }
    free(map);
    map = NULL;
}

void nn_strmap_set(NNStrMap* map, char* name, char* value)
{
    NNStrMap* sub;
    if(map->name == NULL)
    {
        map->name = nn_strbox_make(name, true);
        map->value = (char*)malloc(strlen(value) + 1);
        strcpy(map->value, value);
        map->nxt = NULL;
        return;
    }
    sub = map;
    while(true)
    {
        if(!strcmp(name, sub->name->data))
        {
            if(sub->value != NULL)
            {
                free(sub->value);
                sub->value = NULL;
                sub->value = (char*)malloc(strlen(value) + 1);
                strcpy(sub->value, value);
                return;
            }
        }
        if(sub->nxt == NULL)
        {
            sub->nxt = (NNStrMap*)malloc(sizeof(NNStrMap));
            sub = sub->nxt;
            sub->name = nn_strbox_make(name, true);
            sub->value = (char*)malloc(strlen(value) + 1);
            strcpy(sub->value, value);
            sub->nxt = NULL;
            return;
        }
        sub = sub->nxt;
    }
}

char* nn_strmap_get(NNStrMap* map, char* name)
{
    NNStrMap* sub;
    sub = map;
    while(sub != NULL)
    {
        if(!strcmp(name, sub->name->data))
        {
            return sub->value;
        }
        sub = sub->nxt;
    }
    return "";
}

void display_both(NNStrMap* map, char* first)
{
    printf("%s %s\n", first, nn_strmap_get(map, first));
}

int main(int argc, char** argv)
{
    NNStrMap* test;

    test = nn_strmap_make();
    nn_strmap_set(test, "One", "Won");
    nn_strmap_set(test, "Two", "Too");
    nn_strmap_set(test, "Four", "Fore");

    // display them out of order
    display_both(test, "Two");
    display_both(test, "Four");
    display_both(test, "One");

    printf("\n");

    // reset an existing entry
    nn_strmap_set(test, "Two", "To");

    display_both(test, "Two");
    display_both(test, "Four");
    display_both(test, "One");

    printf("\n");

    display_both(test, "Eight");

    nn_strmap_set(test, "Eight", "Ate");

    printf("\n");

    display_both(test, "Eight");
    nn_strmap_destroy(test);
}
