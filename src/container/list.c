#include "list.h"

#include <stdlib.h>
#include <string.h>

static const struct _object list_object = {
    (void     (*) (void *))         list_delete, 
    (void *   (*) (void *))         list_copy,
    NULL,
    (void     (*) (void *, void *)) list_append_list,
    (json_t * (*) (void *))         list_serialize
};

struct _list * list_create ()
{
    struct _list * list = (struct _list *) malloc(sizeof(struct _list));
    list->object = &list_object;
    list->first = NULL; 
    list->last  = NULL;
    list->size = 0;

    return list;
}


void list_delete (struct _list * list)
{
    struct _list_it * current;
    struct _list_it * next;

    current = list->first;

    while (current != NULL) {
        next = current->next;

        object_delete(current->data);

        free(current);
        
        current = next;
    }

    free(list);
}


json_t * list_serialize (struct _list * list)
{
    json_t * json = json_object();

    json_object_set(json, "ot", json_integer(SERIALIZE_LIST));

    json_t * items = json_array();
    struct _list_it * it;
    for (it = list_iterator(list); it != NULL; it = it->next) {
        json_array_append(items, object_serialize(it->data));
    }

    json_object_set(json, "items", items);

    return json;
}


struct _list * list_deserialize (json_t * json)
{
    json_t * array = json_object_get(json, "items");
    if (! json_is_array(array))
        return NULL;

    struct _list * list = list_create();

    int i;
    for (i = 0; i < json_array_size(array); i++) {
        void * data = deserialize(json_array_get(array, i));
        if (data == NULL) {
            serialize_error = SERIALIZE_LIST;
            object_delete(list);
            return NULL;
        }
        list_append(list, data);
        object_delete(data);
    }

    return list;
}


void list_append (struct _list * list, void * data)
{
    struct _list_it * list_it;

    list_it = (struct _list_it *) malloc(sizeof(struct _list_it));
    list_it->data = object_copy(data);
    list_it->next = NULL;
    list_it->prev = list->last;

    list->size++;
    if (list->first == NULL) {
        list->first = list_it;
        list->last  = list_it;
    }
    else {
        list->last->next = list_it;
        list->last = list_it;
    }

    list->size++;
}


void list_append_list (struct _list * list, struct _list * rhs)
{
    struct _list_it * it;
    for (it = list_iterator(rhs); it != NULL; it = it->next) {
        list_append(list, it->data);
    }
}


struct _list_it * list_iterator (struct _list * list)
{
    return list->first;
}


struct _list * list_copy (struct _list * list)
{
    struct _list * new_list = list_create();
    struct _list_it * it;

    for (it = list_iterator(list); it != NULL; it = it->next) {
        list_append(new_list, it->data);
    }

    return new_list;
}


void * list_first (struct _list * list)
{
    if (list->first == NULL)
        return NULL;
    return list->first->data;
}


struct _list_it * list_remove (struct _list * list, struct _list_it * iterator)
{
    struct _list_it * next = iterator->next;

    if (iterator->prev != NULL)
        iterator->prev->next = iterator->next;

    if (iterator->next != NULL)
        iterator->next->prev = iterator->prev;

    if (list->first == iterator)
        list->first = iterator->next;

    if (list->last == iterator)
        list->last = iterator->prev;
    
    object_delete(iterator->data);
    free(iterator);

    list->size--;

    return next;
}