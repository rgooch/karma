/*LINTLIBRARY*/
/*  main.c

    This code provides Associative Array support.

    Copyright (C) 1995  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for the management of associative
  arrays.


    Written by      Richard Gooch   14-OCT-1995

    Updated by      Richard Gooch   17-DEC-1995: Created <aa_get_all_pairs> and
  <aa_get_pair_info> routines.

    Updated by      Richard Gooch   21-DEC-1995: Fixed bug in <aa_get_pair>
  routine: value was not returned.

    Last updated by Richard Gooch   22-DEC-1995: Added <<front>> parameter to
  <aa_put_pair> routine.


*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <karma.h>
#include <karma_aa.h>
#include <karma_a.h>
#include <karma_m.h>

#define AA_MAGIC_NUMBER 298776298
#define PAIR_MAGIC_NUMBER 2084295498

#define VERIFY_ASSOCARRAY(aa) {if (aa == NULL) \
{(void) fprintf (stderr, "NULL associative array passed\n"); \
 a_prog_bug (function_name); } \
if (aa->magic_number != AA_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid associative array object\n"); \
 a_prog_bug (function_name); } }

#define VERIFY_PAIR(pair) {if (pair == NULL) \
{(void) fprintf (stderr, "NULL associative array pair passed\n"); \
 a_prog_bug (function_name); } \
if (pair->magic_number != PAIR_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid associative array pair object\n"); \
 a_prog_bug (function_name); } }


/*  Private structures  */
struct assocarray_type
{
    unsigned int magic_number;
    void *info;
    int (*key_compare_func) (void *key1, void *key2);
    void *(*key_copy_func) (void *key, uaddr length, flag *ok);
    void (*key_destroy_func) (void *key);
    void *(*value_copy_func) (void *value, uaddr length, flag *ok);
    void (*value_destroy_func) (void *value);
    KAssociativeArrayPair first_pair;
    KAssociativeArrayPair last_pair;
};

struct assocarraypair_type
{
    unsigned int magic_number;
    KAssociativeArray array;
    void *key;
    uaddr key_length;
    void *value;
    uaddr value_length;
    KAssociativeArrayPair next;
    KAssociativeArrayPair prev;
};


/*  Private functions  */


/*  Public functions follow  */


/*PUBLIC_FUNCTION*/
KAssociativeArray aa_create (void *info,
			     int (*key_compare_func) (void *key1, void *key2),
			     void *(*key_copy_func) (void *key,
						     uaddr length, flag *ok),
			     void (*key_destroy_func) (void *key),
			     void *(*value_copy_func) (void *value,
						       uaddr length, flag *ok),
			     void (*value_destroy_func) (void *value))
/*  [PURPOSE] This routine will create a general purpose associative array of
    key-value pairs.
    <info> Arbitrary information to be stored with the associative array.
    <key_compare_func> The function used to compare two keys. The interface to
    this routine is as follows:
    [<pre>]
    int key_compare_func (void *key1, void *key2)
    *   [PURPOSE] This routine will compare two keys.
        <key1> The first key.
	<key2> The second key.
	[RETURNS] 0 if the keys match, a negative number if <<key1>> is less
	than <<key2>>, a positive number if <<key1>> is greater than <<key2>>.
    *
    [</pre>]
    <key_copy_func> The function used to copy keys. The interface to this
    routine is as follows:
    [<pre>]
    void *key_copy_func (void *key, uaddr length, flag *ok)
    *   [PURPOSE] This routine will copy a key.
        <key> The key to copy.
	<length> The length of the key in bytes.
	<ok> The value TRUE will be written here on success, else FALSE is
	written here.
	[RETURNS] A copy of the key on success.
    *
    [</pre>]
    <key_destroy_func> The function used to destroy keys. The interface to this
    routine is as follows:
    [<pre>]
    void key_destroy_func (void *key)
    *   [PURPOSE] This routine will destroy keys.
        <key> The key to destroy.
	[RETURNS] Nothing.
    *
    [</pre>]
    <value_copy_func> The function used to copy values. The interface to this
    routine is as follows:
    [<pre>]
    void *value_copy_func (void *value, uaddr length, flag *ok)
    *   [PURPOSE] This routine will copy a value.
        <value> The value to copy.
	<length> The length of the value in bytes.
	<ok> The value TRUE will be written here on success, else FALSE is
	written here.
	[RETURNS] A copy of the value on success.
    *
    [</pre>]
    <value_destroy_func> The function used to destroy values. The interface to
    this routine is as follows:
    [<pre>]
    void value_destroy_func (void *value)
    *   [PURPOSE] This routine will destroy values.
        <value> The value to destroy.
	[RETURNS] Nothing.
    *
    [</pre>]
    [RETURNS] An associative array on success, else NULL.
*/
{
    KAssociativeArray aa;
    static char function_name[] = "aa_create";

    if ( ( aa = (KAssociativeArray) m_alloc (sizeof *aa) ) == NULL )
    {
	m_error_notify (function_name, "associative array");
    }
    aa->info = info;
    aa->key_compare_func = key_compare_func;
    aa->key_copy_func = key_copy_func;
    aa->key_destroy_func = key_destroy_func;
    aa->value_copy_func = value_copy_func;
    aa->value_destroy_func = value_destroy_func;
    aa->first_pair = NULL;
    aa->last_pair = NULL;
    aa->magic_number = AA_MAGIC_NUMBER;
    return (aa);
}   /*  End Function aa_create  */

/*PUBLIC_FUNCTION*/
void *aa_get_info (KAssociativeArray aa)
/*  [PURPOSE] This routine will get the aribtrary information pointer stored
    with an associative array.
    <aa> The associative array.
    [RETURNS] The arbitrary information pointer.
*/
{
    static char function_name[] = "aa_get_info";

    VERIFY_ASSOCARRAY (aa);
    return (aa->info);
}   /*  End Function aa_get_info  */

/*PUBLIC_FUNCTION*/
KAssociativeArrayPair aa_put_pair (KAssociativeArray aa,
				   void *key, uaddr key_length,
				   void *value, uaddr value_length,
				   unsigned int replacement_policy, flag front)
/*  [PURPOSE] This routine will add a key-value pair to an associative array.
    There must be no existing key-value pair with the same key.
    <aa> The associative array.
    <key> The key.
    <key_length> The length of the key, in bytes. If this is 0 and the
    <<key_copy_func>> is NULL, the key pointer is copied directly. If this is
    greater than zero and the <<key_copy_func>> is NULL, the specified number
    of bytes are copied.
    <value> The value.
    <value_length> The length of the value, in bytes. If this is 0 and the
    <<value_copy_func>> is NULL, the value pointer is copied directly. If this
    is greater than zero and the <<value_copy_func>> is NULL, the specified
    number of bytes are copied.
    <replacement_policy> The policy to use when adding the pair. Legal values:
        KAA_REPLACEMENT_POLICY_NEW         Fail if existing key found
        KAA_REPLACEMENT_POLICY_UPDATE      Fail if no existing key found
        KAA_REPLACEMENT_POLICY_PUT         Add pair, remove old key if exists
    <front> If TRUE and the key is new, place the pair at the front of the
    array. If FALSE and the key is new, place the pair at the end of the
    array.
    [RETURNS] A KAssociativeArrayPair object on success, else NULL.
*/
{
    KAssociativeArrayPair old, new;
    struct assocarraypair_type tmp_pair;
    flag ok;
    void *old_value;
    static char function_name[] = "aa_put_pair";

    VERIFY_ASSOCARRAY (aa);
    old = aa_get_pair (aa, key, &old_value);
    if (old == NULL)
    {
	if (replacement_policy == KAA_REPLACEMENT_POLICY_UPDATE) return (NULL);
    }
    else
    {
	if (replacement_policy == KAA_REPLACEMENT_POLICY_NEW) return (NULL);
    }
    if (old == NULL)
    {
	/*  Create new  */
	if ( ( new = (KAssociativeArrayPair) m_alloc (sizeof *new) ) == NULL )
	{
	    m_error_notify (function_name, "new pair");
	    return (NULL);
	}
    }
    else
    {
	/*  Update old  */
	m_copy ( (char *) &tmp_pair, (char *) old, sizeof tmp_pair );
	tmp_pair.key = NULL;
	tmp_pair.key_length = 0;
	tmp_pair.value = NULL;
	tmp_pair.value_length = 0;
	new = &tmp_pair;
    }
    new->array = aa;
    new->key_length = key_length;
    new->value_length = value_length;
    /*  Copy key  */
    if (aa->key_copy_func == NULL)
    {
	if (key_length == 0)
	{
	    new->key = key;
	    ok = TRUE;
	}
	else
	{
	    if ( ( new->key = (void *) m_dup (key, key_length) ) == NULL )
	    {
		ok = FALSE;
	    }
	    else ok = TRUE;
	}
    }
    else
    {
	new->key = (*aa->key_copy_func) (key, key_length, &ok);
    }
    if (!ok)
    {
	m_error_notify (function_name, "key");
	if (old == NULL) m_free ( (char *) new );
	return (NULL);
    }
    /*  Copy value  */
    if (aa->value_copy_func == NULL)
    {
	if (value_length == 0)
	{
	    new->value = value;
	    ok = TRUE;
	}
	else
	{
	    if ( ( new->value = (void *) m_dup (value,value_length) ) == NULL )
	    {
		ok = FALSE;
	    }
	    else ok = TRUE;
	}
    }
    else
    {
	new->value = (*aa->value_copy_func) (value, value_length, &ok);
    }
    if (!ok)
    {
	m_error_notify (function_name, "value");
	if (aa->key_destroy_func == NULL)
	{
	    if (key_length != 0) m_free ( (char *) new->key );
	}
	else (*aa->key_destroy_func) (new->key);
	if (old == NULL) m_free ( (char *) new );
	return (NULL);
    }
    if (old != NULL)
    {
	/*  Deallocate the old key and value  */
	if (old->array->key_destroy_func == NULL)
	{
	    if (old->key_length != 0) m_free ( (char *) old->key );
	}
	else (*old->array->key_destroy_func) (old->key);
	if (old->array->value_destroy_func == NULL)
	{
	    if (old->value_length != 0) m_free ( (char *) old->value );
	}
	else (*old->array->value_destroy_func) (old->value);
	old->key = tmp_pair.key;
	old->key_length = tmp_pair.key_length;
	old->value = tmp_pair.value;
	old->value_length = tmp_pair.value_length;
	return (old);
    }
    /*  New key: add to array  */
    new->magic_number = PAIR_MAGIC_NUMBER;
    if (front)
    {
	new->prev = NULL;
	new->next = aa->first_pair;
	if (aa->first_pair == NULL)
	{
	    /*  Create the first entry  */
	    aa->last_pair = new;
	}
	else
	{
	    /*  Insert at beginning  */
	    aa->first_pair->prev = new;
	}
	aa->first_pair = new;
    }
    else
    {
	new->prev = aa->last_pair;
	new->next = NULL;
	if (aa->first_pair == NULL)
	{
	    /*  Create the first entry  */
	    aa->first_pair = new;
	}
	else
	{
	    /*  Append to array  */
	    aa->last_pair->next = new;
	}
	aa->last_pair = new;
    }
    return (new);
}   /*  End Function aa_put_pair  */

/*PUBLIC_FUNCTION*/
KAssociativeArrayPair aa_get_pair (KAssociativeArray aa, void *key,
				   void **value)
/*  [PURPOSE] This routine will find a key-value pair in an associative array.
    <aa> The associative array.
    <key> The key.
    <value> The value found will be written here on success.
    [RETURNS] A KAssociativeArrayPair object if the key was found, else NULL.
*/
{
    KAssociativeArrayPair pair;
    static char function_name[] = "aa_get_pair";

    VERIFY_ASSOCARRAY (aa);
    if (aa->first_pair == NULL) return (NULL);
    for (pair = aa->first_pair; pair != NULL; pair = pair->next)
    {
	if ( (*aa->key_compare_func) (key, pair->key) == 0 )
	{
	    *value = pair->value;
	    return (pair);
	}
    }
    return (NULL);
}   /*  End Function aa_get_pair  */

/*PUBLIC_FUNCTION*/
void aa_destroy_pair (KAssociativeArrayPair pair)
/*  [PURPOSE] This routine will remove a key-value pair from an associative
    array and will destroy all storage allocated for the pair.
    <pair> The key-value pair.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "aa_destroy_pair";

    VERIFY_PAIR (pair);
    if (pair->prev != NULL) pair->prev->next = pair->next;
    if (pair->next != NULL) pair->next->prev = pair->prev;
    if (pair->array->first_pair == pair) pair->array->first_pair = pair->next;
    if (pair->array->last_pair == pair) pair->array->last_pair = pair->prev;
    if (pair->array->key_destroy_func == NULL)
    {
	if (pair->key_length != 0) m_free ( (char *) pair->key );
    }
    else (*pair->array->key_destroy_func) (pair->key);
    if (pair->array->value_destroy_func == NULL)
    {
	if (pair->value_length != 0) m_free ( (char *) pair->value );
    }
    else (*pair->array->value_destroy_func) (pair->value);
    m_clear ( (char *) pair, sizeof *pair );
    m_free ( (char *) pair );
}   /*  End Function aa_destroy_pair  */

/*PUBLIC_FUNCTION*/
KAssociativeArrayPair *aa_get_all_pairs (KAssociativeArray aa,
					 unsigned int *num_pairs)
/*  [PURPOSE] This routine will get all the key-value pairs on an associative
    array.
    <aa> The associative array.
    <num_pairs> The number of pairs found is written here. If no pairs are
    found NULL is written here.
    [RETURNS] An array of key-value pairs on success, else NULL. If
    <<num_pairs>> is non-zero, an error occurred.
*/
{
    KAssociativeArrayPair pair;
    unsigned int num;
    KAssociativeArrayPair *pairs;
    static char function_name[] = "aa_get_all_pairs";

    VERIFY_ASSOCARRAY (aa);
    *num_pairs = 0;
    if (aa->first_pair == NULL) return (NULL);
    for (pair = aa->first_pair, num = 0; pair != NULL;
	 pair = pair->next, ++num);
    if ( ( pairs = (KAssociativeArrayPair *) m_alloc (sizeof *pairs * num) )
	== NULL )
    {
	m_error_notify (function_name, "array of pairs");
	*num_pairs = 1;
	return (NULL);
    }
    for (pair = aa->first_pair, num = 0; pair != NULL;
	 pair = pair->next, ++num)
    {
	pairs[num] = pair;
    }
    *num_pairs = num;
    return (pairs);
}   /*  End Function aa_get_all_pairs  */

/*PUBLIC_FUNCTION*/
void aa_get_pair_info (KAssociativeArrayPair pair,
		       void **key, uaddr *key_length,
		       void **value, uaddr *value_length)
/*  [PURPOSE] This routine will get the key and value information for a
    key-value pair in an associative array.
    <pair> The key-value pair.
    <key> The key will be written here.
    <key_length> The length of the key will be written here.
    <value> The value will be written here.
    <value_length> The length of the value will be written here.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "aa_get_pair_info";

    VERIFY_PAIR (pair);
    *key = pair->key;
    *key_length = pair->key_length;
    *value = pair->value;
    *value_length = pair->value_length;
}   /*  End Function aa_get_pair_info  */
