#include "angband.h"

#include <assert.h>

obj_ptr obj_alloc(void)
{
    obj_ptr obj = malloc(sizeof(object_type));
    object_wipe(obj);
    return obj;
}

obj_ptr obj_copy(obj_ptr obj)
{
    obj_ptr copy = malloc(sizeof(object_type));
    assert(obj);
    *copy = *obj;
    return copy;
}

void obj_clear_dun_info(obj_ptr obj)
{
    obj->next_o_idx = 0;
    obj->held_m_idx = 0;
    obj->ix = 0;
    obj->iy = 0;
    obj->marked &= (OM_WORN | OM_COUNTED | OM_EFFECT_COUNTED | OM_EGO_COUNTED | OM_ART_COUNTED);
}

/************************************************************************
 * Predicates
 ***********************************************************************/
bool obj_is_art(obj_ptr obj)
{
    if (obj->name1 || obj->art_name) return TRUE;
    return FALSE;
}

bool obj_is_ego(obj_ptr obj)
{
    if (obj->name2) return TRUE;
    return FALSE;
}

bool obj_exists(obj_ptr obj)
{
    return obj ? TRUE : FALSE;
}

/************************************************************************
 * Sorting
 ***********************************************************************/
void obj_clear_scratch(obj_ptr obj)
{
    if (obj) obj->scratch = 0;
}

static int _obj_cmp_type(obj_ptr obj)
{
    if (!object_is_device(obj))
    {
        if (object_is_fixed_artifact(obj)) return 3;
        else if (obj->art_name) return 2;
        else if (object_is_ego(obj)) return 1;
    }
    return 0;
}

int obj_cmp(obj_ptr left, obj_ptr right)
{
    int left_type, right_type;
    /* Modified from object_sort_comp but the comparison is tri-valued
     * as is standard practice for compare functions. We also memoize
     * computation of obj_value for efficiency. */

    /* Empty slots sort to the end */
    if (!left && !right) return 0;
    if (!left && right) return 1;
    if (left && !right) return -1;

    assert(left && right);

    /* Hack -- readable books always come first (This fails for the Skillmaster) */
    if (left->tval == REALM1_BOOK && right->tval != REALM1_BOOK) return -1;
    if (left->tval != REALM1_BOOK && right->tval == REALM1_BOOK) return 1;

    if (left->tval == REALM2_BOOK && right->tval != REALM2_BOOK) return -1;
    if (left->tval != REALM2_BOOK && right->tval == REALM2_BOOK) return 1;

    /* Objects sort by decreasing type */
    if (left->tval < right->tval) return 1;
    if (left->tval > right->tval) return -1;

    /* Non-aware (flavored) items always come last */
    if (!object_is_aware(left) && object_is_aware(right)) return 1;
    if (object_is_aware(left) && !object_is_aware(right)) return -1;

    /* Objects sort by increasing sval */
    if (left->sval < right->sval) return -1;
    if (left->sval > right->sval) return 1;

    /* Unidentified objects always come last */
    if (!object_is_known(left) && object_is_known(right)) return 1;
    if (object_is_known(left) && !object_is_known(right)) return -1;

    /* Fixed artifacts, random artifacts and ego items */
    left_type = _obj_cmp_type(left);
    right_type = _obj_cmp_type(right);
    if (left_type < right_type) return -1;
    if (left_type > right_type) return 1;

    switch (left->tval)
    {
    case TV_FIGURINE:
    case TV_STATUE:
    case TV_CORPSE:
    case TV_CAPTURE:
        if (r_info[left->pval].level < r_info[right->pval].level) return -1;
        if (r_info[left->pval].level == r_info[right->pval].level && left->pval < right->pval) return -1;
        return 1;

    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
        if (left->to_h + left->to_d < right->to_h + right->to_d) return -1;
        if (left->to_h + left->to_d > right->to_h + right->to_d) return 1;
        break;

    case TV_ROD:
    case TV_WAND:
    case TV_STAFF:
        if (left->activation.type < right->activation.type) return -1;
        if (left->activation.type > right->activation.type) return 1;
        if (device_level(left) < device_level(right)) return -1;
        if (device_level(left) > device_level(right)) return 1;
        break;
    }

    /* Lastly, sort by decreasing value ... but consider stacks */
    if (left->number == 1 && right->number == 1)
    {
        if (!left->scratch) left->scratch = obj_value(left);
        if (!right->scratch) right->scratch = obj_value(right);

        if (left->scratch < right->scratch) return 1;
        if (left->scratch > right->scratch) return -1;
    }
    else
    {
        if (left->number < right->number) return 1;
        if (left->number > right->number) return -1;
    }
    return 0;
}

/************************************************************************
 * Menus
 ***********************************************************************/
char obj_label(obj_ptr obj)
{
    cptr insc;
    if (!obj->inscription) return '\0';
    insc = quark_str(obj->inscription);

    for (insc = strchr(insc, '@'); insc; insc = strchr(insc, '@'))
    {
        insc++;
        /* @mc uses 'c' as a label only for the 'm' command */
        if (command_cmd && *insc == command_cmd)
        {
            insc++;
            if ( ('a' <= *insc && *insc <= 'z')
              || ('A' <= *insc && *insc <= 'Z')
              || ('0' <= *insc && *insc <= '9') )
            {
                return *insc;
            }
        }
        /* @3 uses '3' as a label for *any* command */
        else if ('0' <= *insc && *insc <= '9')
            return *insc;
    }
    return '\0';
}

/************************************************************************
 * Stacking
 ***********************************************************************/
bool obj_can_combine(obj_ptr dest, obj_ptr obj, int options)
{
    /* HISTORIAN: object_similar_part and store_object_similar */
    bool is_store = options & INV_STORE;
    int  i;

    if (dest == obj) return FALSE;
    if (dest->k_idx != obj->k_idx) return FALSE; /* i.e. same tval/sval */
    if (obj_is_art(dest) || obj_is_art(obj)) return FALSE;

    switch (dest->tval)
    {
    /* Objects that never combine */
    case TV_CHEST:
    case TV_CARD:
    case TV_CAPTURE:
    case TV_RUNE:
    case TV_STAFF:
    case TV_WAND:
    case TV_ROD:
        return FALSE;

    case TV_STATUE:
        if (dest->sval != SV_PHOTO) break;
        /* Fall Thru for monster check (Q: Why don't statues with same monster combine?) */
    case TV_FIGURINE:
    case TV_CORPSE:
        if (dest->pval != obj->pval) return FALSE;
        break;

    case TV_FOOD:
    case TV_POTION:
    case TV_SCROLL:
        break;

    /* Equipment */
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_RING:
    case TV_AMULET:
    case TV_LITE:
    case TV_WHISTLE:
        /* Require full knowledge of both items. Ammo skips this check
         * so that you can shoot unidentifed stacks of arrows and have
         * them recombine later. */
        if (!is_store)
        {
            if (!object_is_known(dest) || !object_is_known(obj)) return FALSE;
        }
        /* Fall through */
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
        if (!is_store)
        {
            if (object_is_known(dest) != object_is_known(obj)) return FALSE;
            if (dest->feeling != obj->feeling) return FALSE;
        }

        /* Require identical bonuses */
        if (dest->to_h != obj->to_h) return FALSE;
        if (dest->to_d != obj->to_d) return FALSE;
        if (dest->to_a != obj->to_a) return FALSE;
        if (dest->pval != obj->pval) return FALSE;

        /* Require identical ego types (Artifacts were precluded above) */
        if (dest->name2 != obj->name2) return FALSE;

        /* Require identical added essence  */
        if (dest->xtra3 != obj->xtra3) return FALSE;
        if (dest->xtra4 != obj->xtra4) return FALSE;

        /* Hack -- Never stack "powerful" items (Q: What does this mean?) */
        if (dest->xtra1 || obj->xtra1) return FALSE;

        /* Hack -- Never stack recharging items */
        if (dest->timeout || obj->timeout) return FALSE;

        /* Require identical "values" */
        if (dest->ac != obj->ac) return FALSE;
        if (dest->dd != obj->dd) return FALSE;
        if (dest->ds != obj->ds) return FALSE;

        break;

    default:
        /* Require knowledge */
        if (!object_is_known(dest) || !object_is_known(obj)) return FALSE;
    }

    /* Hack -- Identical art_flags! */
    for (i = 0; i < OF_ARRAY_SIZE; i++)
        if (dest->flags[i] != obj->flags[i]) return FALSE;

    /* Hack -- Require identical "cursed" status */
    if (dest->curse_flags != obj->curse_flags) return FALSE;

    /* Require identical activations */
    if ( dest->activation.type != obj->activation.type
      || dest->activation.cost != obj->activation.cost
      || dest->activation.power != obj->activation.power
      || dest->activation.difficulty != obj->activation.difficulty
      || dest->activation.extra != obj->activation.extra )
    {
        return FALSE;
    }

    /* Hack -- Require identical "broken" status */
    if ((dest->ident & IDENT_BROKEN) != (obj->ident & IDENT_BROKEN)) return FALSE;

    /* Shops always merge inscriptions, but never discounts. For the
     * player, merging of inscriptions and discounts is controlled
     * by options (stack_force_*) */
    if (is_store)
    {
        if (dest->discount != obj->discount) return FALSE;
    }
    else
    {
        /* If both objects are inscribed, then inscriptions must match. */
        if (dest->inscription && obj->inscription && dest->inscription != obj->inscription)
            return FALSE;

        if (!stack_force_notes && dest->inscription != obj->inscription) return FALSE;
        if (!stack_force_costs && dest->discount != obj->discount) return FALSE;
    }

    return TRUE;
}

/* combine obj into dest up to the max stack size.
 * decrease obj->number by the amount combined and 
 * return the amount combined. */
int obj_combine(obj_ptr dest, obj_ptr obj, int options)
{
    /* HISTORIAN: object_absorb and store_object_absorb */
    bool is_store = options & INV_STORE;
    int  amt;

    assert(dest && obj);
    assert(dest->number <= OBJ_STACK_MAX);

    if (!obj_can_combine(dest, obj, options)) return 0;

    if (dest->number + obj->number > OBJ_STACK_MAX)
        amt = OBJ_STACK_MAX - dest->number;
    else
        amt = obj->number;

    dest->number += amt;
    obj->number -= amt;

    if (!is_store)
    {
        if (object_is_known(obj)) obj_identify(dest);

        /* Hack -- clear "storebought" if only one has it */
        if ( ((dest->ident & IDENT_STORE) || (obj->ident & IDENT_STORE))
          && !((dest->ident & IDENT_STORE) && (obj->ident & IDENT_STORE)) )
        {
            if (obj->ident & IDENT_STORE) obj->ident &= ~IDENT_STORE;
            if (dest->ident & IDENT_STORE) dest->ident &= ~IDENT_STORE;
        }

        /* Hack -- blend "inscriptions" */
        if (obj->inscription && !dest->inscription) dest->inscription = obj->inscription;

        /* Hack -- blend "feelings" */
        if (obj->feeling) dest->feeling = obj->feeling;

        /* Hack -- could average discounts XXX XXX XXX */
        /* Hack -- save largest discount XXX XXX XXX */
        if (dest->discount < obj->discount) dest->discount = obj->discount;
    }
    return amt;
}

/************************************************************************
 * Savefiles
 ***********************************************************************/

enum object_save_fields_e {
    SAVE_ITEM_DONE = 0,
    SAVE_ITEM_PVAL,
    SAVE_ITEM_DISCOUNT,
    SAVE_ITEM_NUMBER,
    SAVE_ITEM_NAME1,
    SAVE_ITEM_NAME2,
    SAVE_ITEM_NAME3,
    SAVE_ITEM_ART_NAME,
    SAVE_ITEM_TIMEOUT,
    SAVE_ITEM_COMBAT,
    SAVE_ITEM_ARMOR,
    SAVE_ITEM_DAMAGE_DICE,
    SAVE_ITEM_IDENT,
    SAVE_ITEM_MARKED_BYTE,
    SAVE_ITEM_FEELING,
    SAVE_ITEM_INSCRIPTION,
    SAVE_ITEM_ART_FLAGS_0,
    SAVE_ITEM_ART_FLAGS_1,
    SAVE_ITEM_ART_FLAGS_2,
    SAVE_ITEM_ART_FLAGS_3,
    SAVE_ITEM_ART_FLAGS_4,
    SAVE_ITEM_ART_FLAGS_5,
    SAVE_ITEM_ART_FLAGS_6,
    SAVE_ITEM_ART_FLAGS_7,
    SAVE_ITEM_ART_FLAGS_8,
    SAVE_ITEM_ART_FLAGS_9,
    SAVE_ITEM_CURSE_FLAGS,
    SAVE_ITEM_RUNE_FLAGS,
    SAVE_ITEM_HELD_M_IDX,
    SAVE_ITEM_XTRA1,
    SAVE_ITEM_XTRA2,
    SAVE_ITEM_XTRA3,
    SAVE_ITEM_XTRA4,
    SAVE_ITEM_XTRA5_OLD,
    SAVE_ITEM_ACTIVATION,
    SAVE_ITEM_MULT,
    SAVE_ITEM_MARKED,
    SAVE_ITEM_XTRA5,
    SAVE_ITEM_KNOWN_FLAGS_0,
    SAVE_ITEM_KNOWN_FLAGS_1,
    SAVE_ITEM_KNOWN_FLAGS_2,
    SAVE_ITEM_KNOWN_FLAGS_3,
    SAVE_ITEM_KNOWN_FLAGS_4,
    SAVE_ITEM_KNOWN_FLAGS_5,
    SAVE_ITEM_KNOWN_FLAGS_6,
    SAVE_ITEM_KNOWN_FLAGS_7,
    SAVE_ITEM_KNOWN_FLAGS_8,
    SAVE_ITEM_KNOWN_FLAGS_9,
    SAVE_ITEM_KNOWN_CURSE_FLAGS,
    SAVE_ITEM_LEVEL,
};

void obj_load(obj_ptr obj, savefile_ptr file)
{
    object_kind *k_ptr;
    char         buf[128];

    object_wipe(obj);

    obj->k_idx = savefile_read_s16b(file);
    k_ptr = &k_info[obj->k_idx];
    obj->tval = k_ptr->tval;
    obj->sval = k_ptr->sval;

    obj->loc.where = savefile_read_byte(file);
    obj->loc.x = savefile_read_byte(file);
    obj->loc.y = savefile_read_byte(file);
    obj->loc.slot = savefile_read_s32b(file);

    obj->weight = savefile_read_s16b(file);

    obj->number = 1;

    for (;;)
    {
        byte code = savefile_read_byte(file);
        if (code == SAVE_ITEM_DONE)
            break;

        switch (code)
        {
        case SAVE_ITEM_PVAL:
            obj->pval = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_DISCOUNT:
            obj->discount = savefile_read_byte(file);
            break;
        case SAVE_ITEM_NUMBER:
            obj->number = savefile_read_byte(file);
            break;
        case SAVE_ITEM_NAME1:
            obj->name1 = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_NAME2:
            obj->name2 = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_NAME3:
            obj->name3 = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_TIMEOUT:
            obj->timeout = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_COMBAT:
            obj->to_h = savefile_read_s16b(file);
            obj->to_d = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_ARMOR:
            obj->to_a = savefile_read_s16b(file);
            obj->ac = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_DAMAGE_DICE:
            obj->dd = savefile_read_byte(file);
            obj->ds = savefile_read_byte(file);
            break;
        case SAVE_ITEM_MULT:
            obj->mult = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_IDENT:
            obj->ident = savefile_read_byte(file);
            break;
        case SAVE_ITEM_MARKED_BYTE:
            obj->marked = savefile_read_byte(file);
            break;
        case SAVE_ITEM_MARKED:
            obj->marked = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_ART_FLAGS_0:
            obj->flags[0] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_ART_FLAGS_1:
            obj->flags[1] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_ART_FLAGS_2:
            obj->flags[2] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_ART_FLAGS_3:
            obj->flags[3] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_ART_FLAGS_4:
            obj->flags[4] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_ART_FLAGS_5:
            obj->flags[5] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_CURSE_FLAGS:
            obj->curse_flags = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_FLAGS_0:
            obj->known_flags[0] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_FLAGS_1:
            obj->known_flags[1] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_FLAGS_2:
            obj->known_flags[2] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_FLAGS_3:
            obj->known_flags[3] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_FLAGS_4:
            obj->known_flags[4] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_FLAGS_5:
            obj->known_flags[5] = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_KNOWN_CURSE_FLAGS:
            obj->known_curse_flags = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_RUNE_FLAGS:
            obj->rune = savefile_read_u32b(file);
            break;
        case SAVE_ITEM_HELD_M_IDX:
            obj->held_m_idx = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_XTRA1:
            obj->xtra1 = savefile_read_byte(file);
            break;
        case SAVE_ITEM_XTRA2:
            obj->xtra2 = savefile_read_byte(file);
            break;
        case SAVE_ITEM_XTRA3:
            obj->xtra3 = savefile_read_byte(file);
            break;
        case SAVE_ITEM_XTRA4:
            obj->xtra4 = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_XTRA5_OLD:
            obj->xtra5 = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_XTRA5:
            obj->xtra5 = savefile_read_s32b(file);
            break;
        case SAVE_ITEM_FEELING:
            obj->feeling = savefile_read_byte(file);
            break;
        case SAVE_ITEM_INSCRIPTION:
            savefile_read_cptr(file, buf, sizeof(buf));
            obj->inscription = quark_add(buf);
            break;
        case SAVE_ITEM_ART_NAME:
            savefile_read_cptr(file, buf, sizeof(buf));
            obj->art_name = quark_add(buf);
            break;
        case SAVE_ITEM_ACTIVATION:
            obj->activation.type = savefile_read_s16b(file);
            obj->activation.power = savefile_read_byte(file);
            obj->activation.difficulty = savefile_read_byte(file);
            obj->activation.cost = savefile_read_s16b(file);
            obj->activation.extra = savefile_read_s16b(file);
            break;
        case SAVE_ITEM_LEVEL:
            obj->level = savefile_read_s16b(file);
            break;
        /* default:
            TODO: Report an error back to the load routine!!*/
        }
    }
    if (object_is_device(obj))
        add_flag(obj->flags, OF_ACTIVATE);
}

void obj_save(obj_ptr obj, savefile_ptr file)
{
    savefile_write_s16b(file, obj->k_idx);
    savefile_write_byte(file, obj->loc.where);
    savefile_write_byte(file, obj->loc.x);
    savefile_write_byte(file, obj->loc.y);
    savefile_write_s32b(file, obj->loc.slot);
    savefile_write_s16b(file, obj->weight);
    if (obj->pval)
    {
        savefile_write_byte(file, SAVE_ITEM_PVAL);
        savefile_write_s16b(file, obj->pval);
    }
    if (obj->discount)
    {
        savefile_write_byte(file, SAVE_ITEM_DISCOUNT);
        savefile_write_byte(file, obj->discount);
    }
    if (obj->number != 1)
    {
        savefile_write_byte(file, SAVE_ITEM_NUMBER);
        savefile_write_byte(file, obj->number);
    }
    if (obj->name1)
    {
        savefile_write_byte(file, SAVE_ITEM_NAME1);
        savefile_write_s16b(file, obj->name1);
    }
    if (obj->name2)
    {
        savefile_write_byte(file, SAVE_ITEM_NAME2);
        savefile_write_s16b(file, obj->name2);
    }
    if (obj->name3)
    {
        savefile_write_byte(file, SAVE_ITEM_NAME3);
        savefile_write_s16b(file, obj->name3);
    }
    if (obj->timeout)
    {
        savefile_write_byte(file, SAVE_ITEM_TIMEOUT);
        savefile_write_s16b(file, obj->timeout);
    }
    if (obj->to_h || obj->to_d)
    {
        savefile_write_byte(file, SAVE_ITEM_COMBAT);
        savefile_write_s16b(file, obj->to_h);
        savefile_write_s16b(file, obj->to_d);
    }
    if (obj->to_a || obj->ac)
    {
        savefile_write_byte(file, SAVE_ITEM_ARMOR);
        savefile_write_s16b(file, obj->to_a);
        savefile_write_s16b(file, obj->ac);
    }
    if (obj->dd || obj->ds)
    {
        savefile_write_byte(file, SAVE_ITEM_DAMAGE_DICE);
        savefile_write_byte(file, obj->dd);
        savefile_write_byte(file, obj->ds);
    }
    if (obj->mult)
    {
        savefile_write_byte(file, SAVE_ITEM_MULT);
        savefile_write_s16b(file, obj->mult);
    }
    if (obj->ident)
    {
        savefile_write_byte(file, SAVE_ITEM_IDENT);
        savefile_write_byte(file, obj->ident);
    }
    if (obj->marked)
    {
        savefile_write_byte(file, SAVE_ITEM_MARKED);
        savefile_write_u32b(file, obj->marked);
    }
    if (obj->flags[0])
    {
        savefile_write_byte(file, SAVE_ITEM_ART_FLAGS_0);
        savefile_write_u32b(file, obj->flags[0]);
    }
    if (obj->flags[1])
    {
        savefile_write_byte(file, SAVE_ITEM_ART_FLAGS_1);
        savefile_write_u32b(file, obj->flags[1]);
    }
    if (obj->flags[2])
    {
        savefile_write_byte(file, SAVE_ITEM_ART_FLAGS_2);
        savefile_write_u32b(file, obj->flags[2]);
    }
    if (obj->flags[3])
    {
        savefile_write_byte(file, SAVE_ITEM_ART_FLAGS_3);
        savefile_write_u32b(file, obj->flags[3]);
    }
    if (obj->flags[4])
    {
        savefile_write_byte(file, SAVE_ITEM_ART_FLAGS_4);
        savefile_write_u32b(file, obj->flags[4]);
    }
    if (obj->flags[5])
    {
        savefile_write_byte(file, SAVE_ITEM_ART_FLAGS_5);
        savefile_write_u32b(file, obj->flags[5]);
    }
    if (obj->curse_flags)
    {
        savefile_write_byte(file, SAVE_ITEM_CURSE_FLAGS);
        savefile_write_u32b(file, obj->curse_flags);
    }
    if (obj->known_flags[0])
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_FLAGS_0);
        savefile_write_u32b(file, obj->known_flags[0]);
    }
    if (obj->known_flags[1])
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_FLAGS_1);
        savefile_write_u32b(file, obj->known_flags[1]);
    }
    if (obj->known_flags[2])
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_FLAGS_2);
        savefile_write_u32b(file, obj->known_flags[2]);
    }
    if (obj->known_flags[3])
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_FLAGS_3);
        savefile_write_u32b(file, obj->known_flags[3]);
    }
    if (obj->known_flags[4])
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_FLAGS_4);
        savefile_write_u32b(file, obj->known_flags[4]);
    }
    if (obj->known_flags[5])
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_FLAGS_5);
        savefile_write_u32b(file, obj->known_flags[5]);
    }
    if (obj->known_curse_flags)
    {
        savefile_write_byte(file, SAVE_ITEM_KNOWN_CURSE_FLAGS);
        savefile_write_u32b(file, obj->known_curse_flags);
    }
    if (obj->rune)
    {
        savefile_write_byte(file, SAVE_ITEM_RUNE_FLAGS);
        savefile_write_u32b(file, obj->rune);
    }
    if (obj->held_m_idx)
    {
        savefile_write_byte(file, SAVE_ITEM_HELD_M_IDX);
        savefile_write_s16b(file, obj->held_m_idx);
    }
    if (obj->xtra1)
    {
        savefile_write_byte(file, SAVE_ITEM_XTRA1);
        savefile_write_byte(file, obj->xtra1);
    }
    if (obj->xtra2)
    {
        savefile_write_byte(file, SAVE_ITEM_XTRA2);
        savefile_write_byte(file, obj->xtra2);
    }
    if (obj->xtra3)
    {
        savefile_write_byte(file, SAVE_ITEM_XTRA3);
        savefile_write_byte(file, obj->xtra3);
    }
    if (obj->xtra4)
    {
        savefile_write_byte(file, SAVE_ITEM_XTRA4);
        savefile_write_s16b(file, obj->xtra4);
    }
    if (obj->xtra5)
    {
        savefile_write_byte(file, SAVE_ITEM_XTRA5);
        savefile_write_s32b(file, obj->xtra5);
    }
    if (obj->feeling)
    {
        savefile_write_byte(file, SAVE_ITEM_FEELING);
        savefile_write_byte(file, obj->feeling);
    }
    if (obj->inscription)
    {
        savefile_write_byte(file, SAVE_ITEM_INSCRIPTION);
        savefile_write_cptr(file, quark_str(obj->inscription));
    }
    if (obj->art_name)
    {
        savefile_write_byte(file, SAVE_ITEM_ART_NAME);
        savefile_write_cptr(file, quark_str(obj->art_name));
    }
    if (obj->activation.type)
    {
        savefile_write_byte(file, SAVE_ITEM_ACTIVATION);
        savefile_write_s16b(file, obj->activation.type);
        savefile_write_byte(file, obj->activation.power);
        savefile_write_byte(file, obj->activation.difficulty);
        savefile_write_s16b(file, obj->activation.cost);
        savefile_write_s16b(file, obj->activation.extra);
    }
    if (obj->level)
    {
        savefile_write_byte(file, SAVE_ITEM_LEVEL);
        savefile_write_s16b(file, obj->level);
    }

    savefile_write_byte(file, SAVE_ITEM_DONE);
}
