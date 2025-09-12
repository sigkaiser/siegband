#include "angband.h"

static cptr _desc =
    "The Adeptus Astartes are transhuman warriors on an unending crusade. Their power armor can be "
    "infused with Warp essences extracted from found armor, enhancing their abilities and granting new powers. "
    "They are rarely hindered by invisibility and gain resistance to various forms of damage as they adapt "
    "to the challenges they face. Their stats reflect their superhuman nature, and their combat skills "
    "offer a strong mix of offense, defense, and utility. The Adeptus Astartes are a versatile and powerful "
    "race, though their progression requires effort and dedication.\n \n"
    "The Adeptus Astartes have a humanoid body type; they use the same equipment slots as most normal "
    "player races and excel in both melee and ranged combat. Strength is their primary stat, while wisdom "
    "or intelligence supports those with psychic abilities.";

static void _birth(void)
{
    object_type    forge;

    p_ptr->current_r_idx = MON_ASTARTES;

    object_prep(&forge, lookup_kind(TV_POTION, SV_POTION_HEALING));
    py_birth_obj(&forge);

    object_prep(&forge, lookup_kind(TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL));
    py_birth_obj(&forge);

    object_prep(&forge, lookup_kind(TV_HAFTED, SV_MACE));
    py_birth_obj(&forge);

    py_birth_food();
    py_birth_light();
}

/******************************************************************************
 *              10           20        30        40        45          50
 * Angel: Angel -> Archangel -> Cherub -> Seraph -> Archon -> Planetar -> Solar
 ******************************************************************************/
static void _psycho_spear_spell(int cmd, variant *res)
{
    switch (cmd)
    {
    case SPELL_NAME:
        var_set_string(res, "Psycho-Spear");
        break;
    case SPELL_DESC:
        var_set_string(res, "Fires a beam of pure energy which penetrate the invulnerability barrier.");
        break;
    case SPELL_INFO:
        var_set_string(res, info_damage(1, spell_power(p_ptr->lev * 3), spell_power(p_ptr->lev * 3 + p_ptr->to_d_spell)));
        break;
    case SPELL_CAST:
    {
        int dir = 0;
        var_set_bool(res, FALSE);
        if (!get_fire_dir(&dir)) return;
        fire_beam(GF_PSY_SPEAR, dir, spell_power(randint1(p_ptr->lev*3) + p_ptr->lev*3 + p_ptr->to_d_spell));
        var_set_bool(res, TRUE);
        break;
    }
    default:
        default_spell(cmd, res);
        break;
    }
}

static spell_info _get_spells[] = {
    {  1,  1, 30, punishment_spell},
    {  2,  2, 30, bless_spell},
    {  3,  2, 30, confuse_spell},
    {  5,  3, 30, scare_spell},
    {  7,  4, 30, detect_monsters_spell},
    { 10,  7, 40, heroism_spell},
    { 15, 10, 40, magic_mapping_spell},
    { 20, 15, 50, mana_bolt_I_spell},
    { 23, 15, 50, brain_smash_spell},
    { 25, 17, 50, haste_self_spell},
    { 27, 20, 50, protection_from_evil_spell},
    { 29, 20, 60, dispel_evil_spell},
    { 31, 20, 60, teleport_other_spell},
    { 32, 20, 60, healing_I_spell},
    { 35, 25, 60, destruction_spell},
    { 37, 25, 60, summon_monsters_spell},
    { 40, 30, 65, _psycho_spear_spell},
    { 42, 30, 65, restoration_spell},
    { 45, 80, 65, clairvoyance_spell},
    { 47, 60, 70, summon_angel_spell},
    { 49, 90, 70, summon_hi_dragon_spell},
    { 50, 50, 65, starburst_II_spell},
    { -1, -1, -1, NULL}
};

static void _calc_bonuses(void) {
    /* cf calc_torch in xtra1.c for the 'extra light' */

    p_ptr->align += 200;
    p_ptr->levitation = TRUE;
    res_add(RES_POIS);

    if (p_ptr->lev >= 10)
    {
        res_add(RES_FIRE);
        p_ptr->see_inv++;
    }
    if (p_ptr->lev >= 20)
    {
        p_ptr->pspeed += 1;
        res_add(RES_COLD);
    }
    if (p_ptr->lev >= 30)
    {
        p_ptr->pspeed += 1;
        res_add(RES_ACID);
        res_add(RES_ELEC);
        res_add(RES_CONF);
    }
    if (p_ptr->lev >= 40)
    {
        p_ptr->pspeed += 3;
        p_ptr->reflect = TRUE;
        res_add(RES_TELEPORT);
        res_add(RES_LITE);
    }
    if (p_ptr->lev >= 45)
    {
        p_ptr->pspeed += 1;
    }
    if (p_ptr->lev >= 50)
    {
        p_ptr->pspeed += 1;
    }
}
static void _get_flags(u32b flgs[OF_ARRAY_SIZE]) {
    add_flag(flgs, OF_LEVITATION);
    add_flag(flgs, OF_RES_POIS);

    if (p_ptr->lev >= 10)
    {
        add_flag(flgs, OF_RES_FIRE);
        add_flag(flgs, OF_SEE_INVIS);
    }
    if (p_ptr->lev >= 20)
    {
        add_flag(flgs, OF_SPEED);
        add_flag(flgs, OF_RES_COLD);
    }
    if (p_ptr->lev >= 30)
    {
        add_flag(flgs, OF_RES_ACID);
        add_flag(flgs, OF_RES_ELEC);
        add_flag(flgs, OF_RES_CONF);
    }
    if (p_ptr->lev >= 40)
    {
        add_flag(flgs, OF_REFLECT);
        add_flag(flgs, OF_RES_LITE);
    }
    if (p_ptr->lev >= 45)
    {
    }
    if (p_ptr->lev >= 50)
    {
    }
}

static race_t *_astartes_template(int which, cptr name, cptr desc)
{
    static race_t me[ASTARTES_MAX] = {{0}};
    static bool   init[ASTARTES_MAX] = {FALSE};
    race_t       *r = &me[which];

    if (!init[which])
    {           /* dis, dev, sav, stl, srh, fos, thn, thb */
    skills_t bs = { 25,  35,  40,   3,  18,  12,  48,  35};
    skills_t xs = {  7,  13,  15,   0,   0,   0,  18,  13};

        r->skills = bs;
        r->extra_skills = xs;

        r->infra = 3;
        r->exp = 325; /* 14.6 Mxp */
        r->base_hp = 26;

        r->get_spells = _get_spells;
        r->calc_bonuses = _calc_bonuses;
        r->get_flags = _get_flags;
        init[which] = TRUE;
    }

    r->subname = name;
    r->subdesc = desc;
    r->stats[A_STR] = 1 + p_ptr->lev/10;
    r->stats[A_INT] = 1 + p_ptr->lev/10;
    r->stats[A_WIS] = 4 + p_ptr->lev/10;
    r->stats[A_DEX] = 1 + p_ptr->lev/10;
    r->stats[A_CON] = 1 + p_ptr->lev/10;
    r->stats[A_CHR] = 1 + p_ptr->lev/8;
    r->life = 90 + p_ptr->lev;

    return r;
}

static race_t *_ultramarine_get_race_t(void)
{
    return _astartes_template(ASTARTES_ULTRAMARINE, "Ultramarine", "Placeholder for the Ultramarine chapter.");
}

static race_t *_greyknight_get_race_t(void)
{
    return _astartes_template(ASTARTES_GREY_KNIGHT, "Grey Knight", "Placeholder for the Grey Knight chapter.");
}

static race_t *_blacktemplar_get_race_t(void)
{
    return _astartes_template(ASTARTES_BLACK_TEMPLAR, "Black Templar", "Placeholder for the Black Templar chapter.");
}

static race_t *_thousandsons_get_race_t(void)
{
    return _astartes_template(ASTARTES_THOUSAND_SONS, "Thousand Sons", "Placeholder for the Thousand Sons chapter.");
}

static race_t *_blacklegion_get_race_t(void)
{
    return _astartes_template(ASTARTES_BLACK_LEGION, "Black Legion", "Placeholder for the Black Legion chapter.");
}

static caster_info * _caster_info(void)
{
    static caster_info me = {0};
    static bool init = FALSE;
    if (!init)
    {
        me.magic_desc = "divine power";
        me.which_stat = A_WIS;
        me.encumbrance.max_wgt = 450;
        me.encumbrance.weapon_pct = 67;
        me.encumbrance.enc_wgt = 800;
        init = TRUE;
    }
    return &me;
}

/**********************************************************************
 * Public
 **********************************************************************/
race_t *mon_astartes_get_race(int psubrace)
{
    race_t *result = NULL;

    switch (psubrace)
    {
    case ASTARTES_ULTRAMARINE:
        result = _ultramarine_get_race_t();
        break;
    case ASTARTES_GREY_KNIGHT:
        result = _greyknight_get_race_t();
        break;
    case ASTARTES_BLACK_TEMPLAR:
        result = _blacktemplar_get_race_t();
        break;
    case ASTARTES_THOUSAND_SONS:
        result = _thousandsons_get_race_t();
        break;
    case ASTARTES_BLACK_LEGION:
        result = _blacklegion_get_race_t();
        break;
    default: /* Birth Menus */
        result = _greyknight_get_race_t();
    }

    result->name = "Astartes";
    result->desc = _desc;
    result->flags = RACE_IS_MONSTER;
    result->birth = _birth;
    result->caster_info = _caster_info;
    result->pseudo_class_idx = CLASS_PRIEST;
    result->shop_adjust = 90;

    result->boss_r_idx = MON_RAPHAEL;

    return result;
}
