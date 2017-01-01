#include "angband.h"

#include <assert.h>

/************************************************************************
 * The Skill System
 ***********************************************************************/
#define _MAX_SKILLS 15

enum _type_e {
    _TYPE_NONE = 0,
    _TYPE_MELEE,
    _TYPE_SHOOT,
    _TYPE_MAGIC,
    _TYPE_PRAYER,
    _TYPE_SKILLS,
    _TYPE_TECHNIQUE,
    _TYPE_ABILITY,
};

/* It is convenient to use TV_* and REALM_* for various skills,
 * so other values must be chosen so as not to cause collisions */
enum {
    _SKILL_START = 300,
    _MARTIAL_ARTS = 300,
    _ARCHERY,
    _THROWING,
    _DUAL_WIELDING,
    _RIDING,
    _DEVICES,
    _STEALTH,
    _SPEED,
    _AWARENESS,
    _HEALTH,
    _AGILITY,
    _MAGIC_RESISTANCE,
    _SKILL_STOP
};

/* Abilities are one time purchases granting a power or some other
 * attribute, like regeneration */
enum {
    _ABILITY_START = 400,
    _CLEAR_MIND = 400,
    _EAT_MAGIC,
    _LOREMASTER,
    _LUCK,
    _MASSACRE,
    _PANIC_HIT,
    _REGENERATION,
    _RESISTANCE,
    _STONE_SKIN,
    _ABILITY_STOP
};

typedef struct _skill_s {
    int  type;
    int  subtype;
    cptr name;
    int  max;
    int  current;
} _skill_t, *_skill_ptr;

typedef struct _group_s {
    int  type;
    cptr name;
    cptr desc;
    _skill_t skills[_MAX_SKILLS];
} _group_t, *_group_ptr;

static _group_t _groups[] = {
    { _TYPE_MELEE, "Melee",
        "<color:B>Melee</color> skills increase your ability to fight monsters in hand to hand combat. "
        "The total number of points spent in this group will determine your base fighting "
        "skill, as well as how much melee skill you gain with experience. In addition, this "
        "total gives bonuses to STR and DEX. The individual skills determine your base "
        "proficiency with the given type of weapon, as well as the maximum number of blows "
        "per round you may gain. For martial arts, the number of points determines your "
        "effective fighting level which is used to determine which types of hits you land.",
        {{ _TYPE_MELEE, TV_SWORD, "Swords", 5, 0 },
         { _TYPE_MELEE, TV_POLEARM, "Polearms", 5, 0 },
         { _TYPE_MELEE, TV_HAFTED, "Hafted", 5, 0 },
         { _TYPE_MELEE, TV_DIGGING, "Diggers", 5, 0 },
         { _TYPE_MELEE, _MARTIAL_ARTS, "Martial Arts", 5, 0 },
         { 0 }}},
    { _TYPE_SHOOT, "Ranged",
        "<color:B>Ranged</color> skills increase your ability to fight monsters using distance weapons "
        "such as bows and slings, as well as the ability to throw melee weapons "
        "effectively. The total number of points spent in this group will determine "
        "your base shooting skill, as well as how fast this skill increases with "
        "experience. Archery grants proficiency with the various classes of shooters "
        "as well as increasing your number of shots per round. Throwing grants the power "
        "to throw your leading melee weapon and perhaps have it return to you as well.",
        {{ _TYPE_SHOOT, _ARCHERY, "Archery", 5, 0 },
         { _TYPE_SHOOT, _THROWING, "Throwing", 5, 0 },
         { 0 }}},
    { _TYPE_MAGIC, "Magic",
        "<color:B>Magic</color> skills grant access to various spellcasting realms using INT rather "
        "than WIS as the spellcasting stat. Each point in a given realm increases "
        "your maximum spellcasting level as well as the decreasing the cost and fail "
        "rates of individual spells. You need not learn individual spells the way a "
        "mage might. Rather, you may cast any spell in a known realm provided your "
        "spellcasting level is high enough. The total number of points spent in this "
        "group provides a modest boost to device skills as well as increasing your "
        "INT. Unfortunately. this total also has a negative impact on your life rating, "
        "STR and CON.",
        {{ _TYPE_MAGIC, REALM_ARCANE, "Arcane", 5, 0 },
         { _TYPE_MAGIC, REALM_ARMAGEDDON, "Armageddon", 5, 0 },
         { _TYPE_MAGIC, REALM_CHAOS, "Chaos", 5, 0 },
         { _TYPE_MAGIC, REALM_CRAFT, "Craft", 5, 0 },
         { _TYPE_MAGIC, REALM_CRUSADE, "Crusade", 5, 0 },
         { _TYPE_MAGIC, REALM_DAEMON, "Daemon", 5, 0 },
         { _TYPE_MAGIC, REALM_DEATH, "Death", 5, 0 },
         { _TYPE_MAGIC, REALM_NATURE, "Nature", 5, 0 },
         { _TYPE_MAGIC, REALM_SORCERY, "Sorcery", 5, 0 },
         { _TYPE_MAGIC, REALM_TRUMP, "Trump", 5, 0 },
         { 0 }}},
    { _TYPE_PRAYER, "Prayer",
        "<color:B>Prayer</color> skills work like Magic skills, granting access to various realms of "
        "magic. However, prayer realms use WIS for the spellcasting stat rather than "
        "INT. Moreover, the total points in the Prayer group grants enhanced saving "
        "throws as well as increased WIS, but does not affect device skills or life "
        "rating. As in Magic, the number of points invested in a particular realm "
        "determines your maximum casting level and also influences casting costs and "
        "fail rates. Life magic is only available as a Prayer talent. The other prayer "
        "realms are duplicated as Magic talents, but you can only learn them in one "
        "way: as either INT based or WIS based.",
        {{ _TYPE_PRAYER, REALM_CRUSADE, "Crusade", 5, 0 },
         { _TYPE_PRAYER, REALM_DAEMON, "Daemon", 5, 0 },
         { _TYPE_PRAYER, REALM_DEATH, "Death", 5, 0 },
         { _TYPE_PRAYER, REALM_LIFE, "Life", 5, 0 },
         { 0 }}},
    { _TYPE_SKILLS, "Skills",
        "There are various <color:B>Skills</color> important for every player, including "
        "stealth, perception, health and magic devices. While these skills may be influenced by "
        "talents in other groups (e.g. Magic increases your device skills), it is often "
        "desirable to directly increase these skills even more. In addition, you can directly "
        "improve your starting stats by investing in some of the skills in this group "
        "(e.g. Agility improves DEX while Health improve CON).",
        {{ _TYPE_SKILLS, _AGILITY, "Agility", 3, 0 },
         { _TYPE_SKILLS, _AWARENESS, "Awareness", 3, 0 },
         { _TYPE_SKILLS, _DEVICES, "Devices", 3, 0 },
         { _TYPE_SKILLS, _HEALTH, "Health", 3, 0 },
         { _TYPE_SKILLS, _MAGIC_RESISTANCE, "Magic Resistance", 3, 0 },
         { _TYPE_SKILLS, _SPEED, "Speed", 3, 0 },
         { _TYPE_SKILLS, _STEALTH, "Stealth", 3, 0 },
         { 0 }}},
    { _TYPE_TECHNIQUE, "Techniques",
        "There are various specialty <color:B>Techniques</color> which you may want to "
        "learn, including the Riding and Dual Wielding talents, as well as access to the "
        "Burglary and Kendo techniques. These latter two function like spell realms, using "
        "DEX and WIS respectively as their primary spell stat. Kendo also makes your mana "
        "function like that of the Samurai, and this behavior is used no matter what other "
        "Magic or Prayer realms you have learned. Note that Kendo still requires you to "
        "carry the appropriate spellbooks since you really are not samurai.",
        {{ _TYPE_TECHNIQUE, REALM_BURGLARY, "Burglary", 5, 0 },
         { _TYPE_TECHNIQUE, REALM_HISSATSU, "Kendo", 5, 0 }, 
         { _TYPE_TECHNIQUE, _DUAL_WIELDING, "Dual Wielding", 5, 0 },
         { _TYPE_TECHNIQUE, _RIDING, "Riding", 5, 0 },
         { 0 }}},
    { _TYPE_ABILITY, "Abilities",
        "Many <color:B>Abilities</color> can be purchased for a single skill point. Often "
        "these grant an extra power which you may activate at will without cost, and usually "
        "without fail. Alternatively, abilities may grant a bonus such as regeneration or "
        "good fortune.",
        {{ _TYPE_ABILITY, _CLEAR_MIND, "Clear Mind", 1, 0 },
         { _TYPE_ABILITY, _EAT_MAGIC, "Eat Magic", 1, 0 },
         { _TYPE_ABILITY, _LOREMASTER, "Loremastery", 1, 0 },
         { _TYPE_ABILITY, _LUCK, "Luck", 1, 0 },
         { _TYPE_ABILITY, _MASSACRE, "Massacre", 1, 0 },
         { _TYPE_ABILITY, _PANIC_HIT, "Panic Hit", 1, 0 },
         { _TYPE_ABILITY, _REGENERATION, "Regeneration", 1, 0 },
         { _TYPE_ABILITY, _RESISTANCE, "Resistance", 1, 0 },
         { _TYPE_ABILITY, _STONE_SKIN, "Stone Skin", 1 , 0 },
         { 0 }}},
    { 0 }
};

static _group_ptr _get_group(int id)
{
    int i;
    for (i = 0; ; i++)
    {
        if (!_groups[i].type) return NULL;
        if (_groups[i].type == id) return &_groups[i];
    }
}

static int _get_group_ct(void)
{
    int i;
    int ct = 0;
    for (i = 0; ; i++)
    {
        if (!_groups[i].type) break;
        ct++;
    }
    return ct;
}

static _skill_ptr _get_skill_aux(_group_ptr g, int subtype)
{
    int i;
    for (i = 0; ; i++)
    {
        if (!g->skills[i].type) return NULL;
        if (g->skills[i].subtype == subtype) return &g->skills[i];
    }
}

static _skill_ptr _get_skill(int type, int subtype)
{
    _group_ptr g = _get_group(type);
    assert(g);
    return _get_skill_aux(g, subtype);
}

static int _get_group_pts_aux(_group_ptr g)
{
    int i, ct = 0;
    for (i = 0; i < _MAX_SKILLS; i++)
    {
        if (!g->skills[i].type) break;
        ct += g->skills[i].current;
    }
    return ct;
}

static int _get_group_pts(int id)
{
    _group_ptr g = _get_group(id);
    assert(g);
    return _get_group_pts_aux(g);
}

static int _get_pts(void)
{
    int i, ct = 0;
    for (i = 0; ; i++)
    {
        _group_ptr g = &_groups[i];
        if (!g->type) break;
        ct += _get_group_pts_aux(g);
    }
    return ct;
}

static int _get_max_pts(void)
{
    return 5 + p_ptr->lev/5;
}

static int _get_free_pts(void)
{
    return MAX(0, _get_max_pts() - _get_pts());
}

int skillmaster_new_skills(void)
{
    return _get_free_pts();
}

static int _get_skill_pts(int type, int subtype)
{
    _skill_ptr s = _get_skill(type, subtype);
    assert(s);
    return s->current;
}

static int _get_skill_ct_aux(_group_ptr g)
{
    int i, ct = 0;
    for (i = 0; ; i++)
    {
        _skill_ptr s = &g->skills[i];
        if (!s->type) break;
        ct++;
    }
    return ct;
}

static int _get_skill_ct(int type)
{
    _group_ptr g = _get_group(type);
    assert(g);
    return _get_skill_ct_aux(g);
}

static void _reset_group(_group_ptr g)
{
    int i;
    for (i = 0; ; i++)
    {
        _skill_ptr s = &g->skills[i];
        if (!s->type) break;
        s->current = 0;
    }
}

static void _reset_groups(void)
{
    int i;
    for (i = 0; ; i++)
    {
        _group_ptr g = &_groups[i];
        if (!g->type) break;
        _reset_group(g);
    }
}

static void _load_group(savefile_ptr file,_group_ptr g)
{
    for (;;)
    {
        int subtype = savefile_read_u16b(file);
        if (subtype == 0xFFFE) break;
        _get_skill_aux(g, subtype)->current = savefile_read_u16b(file);
    }
}

static void _load_player(savefile_ptr file)
{
    _reset_groups();

    for (;;)
    {
        int type = savefile_read_u16b(file);
        if (type == 0xFFFF) break;
        _load_group(file, _get_group(type));
    }
}

static void _save_player(savefile_ptr file)
{
    int i, j;
    for (i = 0; ; i++)
    {
        _group_ptr g = &_groups[i];
        if (!g->type) break;
        savefile_write_u16b(file, g->type);
        for (j = 0; ; j++)
        {
            _skill_ptr s = &g->skills[j];
            if (!s->type) break;
            savefile_write_u16b(file, s->subtype);
            savefile_write_u16b(file, s->current);
        }
        savefile_write_u16b(file, 0xFFFE);
    }
    savefile_write_u16b(file, 0xFFFF);
}

/************************************************************************
 * Gain a Skill (UI)
 ***********************************************************************/
static doc_ptr _doc = NULL;

static int _inkey(void)
{
    return inkey_special(TRUE);
}

static void _sync_term(doc_ptr doc)
{
    rect_t r = ui_screen_rect();
    Term_load();
    doc_sync_term(doc, doc_range_top_lines(doc, r.cy), doc_pos_create(r.x, r.y));
}

static int _confirm_skill_ui(_skill_ptr s)
{
    for (;;)
    {
        int cmd;

        doc_clear(_doc);
        doc_printf(_doc, "<color:v>Warning</color>: <indent>You are about to learn the <color:B>%s</color> skill. "
                         "Are you sure you want to do this? The skills you learn are permanent choices and you "
                         "won't be able to change your mind later!</indent>\n", s->name);
        doc_newline(_doc);
        doc_insert(_doc, "<color:y>RET</color>) <color:v>Accept</color>\n");
        doc_insert(_doc, "<color:y>ESC</color>) Cancel\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\r') return UI_OK;
    }
}

static bool _can_gain_skill(_skill_ptr s)
{
    if (!_get_free_pts()) return FALSE;
    if (s->current == s->max) return FALSE;
    if (s->type == _TYPE_MAGIC)
    {
        _skill_ptr s2 = _get_skill(_TYPE_PRAYER, s->subtype);
        if (s2 && s2->current) return FALSE;
    }
    else if (s->type == _TYPE_PRAYER)
    {
        _skill_ptr s2 = _get_skill(_TYPE_MAGIC, s->subtype);
        if (s2 && s2->current) return FALSE;
    }
    return TRUE;
}

static int _gain_skill_ui(_group_ptr g)
{
    int cmd = 0;
    REPEAT_PULL(&cmd);
    for (;;)
    {
        int     i, ct = _get_skill_ct_aux(g);
        int     free_pts = _get_free_pts();
        int     group_pts = _get_group_pts_aux(g);
        doc_ptr cols[2];

        if (!cmd) /* repeat? */
        {
            cols[0] = doc_alloc(40);
            cols[1] = doc_alloc(30);

            doc_clear(_doc);
            doc_printf(_doc, "%s\n\n", g->desc);
            doc_insert(cols[0], "<color:G>Choose the Skill</color>\n");
            for (i = 0; ; i++, ct++)
            {
                _skill_ptr s = &g->skills[i];
                if (!s->type) break;
                doc_printf(cols[0], "  <color:%c>%c</color>) %s",
                    _can_gain_skill(s) ? 'y' : 'D',
                    I2A(i),
                    s->name
                );
                if (s->current && s->max)
                    doc_printf(cols[0], " (%d%%)", 100*s->current/s->max);
                doc_newline(cols[0]);
            }
            doc_printf(cols[1], "%-10.10s: <color:%c>%d</color>\n", "Learned", group_pts ? 'G' : 'r', group_pts);
            doc_printf(cols[1], "%-10.10s: <color:%c>%d</color>\n", "Available", free_pts ? 'G' : 'r', free_pts);
            doc_insert(cols[1], "\n\n");
            doc_insert(cols[1], "  <color:y>?</color>) Help\n");
            doc_insert(cols[1], "<color:y>ESC</color>) Cancel\n");
            doc_insert_cols(_doc, cols, 2, 1);
            doc_free(cols[0]);
            doc_free(cols[1]);

            _sync_term(_doc);
            cmd = _inkey();
        }
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '?') doc_display_help("Skillmaster.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < ct)
            {
                _skill_ptr s = &g->skills[i];
                doc_display_help("Skillmaster.txt", s->name);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < ct)
            {
                _skill_ptr s = &g->skills[i];
                if (_can_gain_skill(s))
                {
                    REPEAT_PUSH(cmd);
                    if(_confirm_skill_ui(s) == UI_OK)
                    {
                        s->current++;
                        return UI_OK;
                    }
                    REPEAT_POP();
                }
            }
        }
        cmd = 0;
    }
}

static int _gain_type_ui(void)
{
    int cmd = 0;
    REPEAT_PULL(&cmd);
    for (;;)
    {
        int i, pts, ct = _get_group_ct();
        int free_pts = _get_free_pts();
        int learned_pts = _get_pts();
        doc_ptr cols[2];

        if (!cmd) /* repeat? */
        {
            cols[0] = doc_alloc(40);
            cols[1] = doc_alloc(30);

            doc_clear(_doc);
            doc_insert(_doc,
                "You must choose which <color:B>skills</color> to learn. The various skills are grouped "
                "into categories. Often, the total number of points in a given group determines broad level "
                "skills and perhaps grants boosts to your starting stats, while the specific skill points "
                "grant targetted benefits (such as proficiency with a class of weapons, or access to "
                "a specific spell realm). Details are given in the submenu for each group. Feel free to "
                "explore the various groups since you may always <color:keypress>ESC</color> to return here.\n\n");
            doc_insert(cols[0], "<color:G>Choose the Skill Type</color>\n");
            for (i = 0; ; i++)
            {
                _group_ptr g = &_groups[i];
                if (!g->type) break;
                pts = _get_group_pts_aux(g);
                doc_printf(cols[0], "  <color:y>%c</color>) %s", I2A(i), g->name);
                if (pts) doc_printf(cols[0], " (%d)", pts);
                doc_newline(cols[0]);
            }

            doc_printf(cols[1], "%-10.10s: <color:%c>%d</color>\n", "Learned", learned_pts ? 'G' : 'r', learned_pts);
            doc_printf(cols[1], "%-10.10s: <color:%c>%d</color>\n", "Available", free_pts ? 'G' : 'r', free_pts);
            doc_insert(cols[1], "\n\n");
            doc_insert(cols[1], "  <color:y>?</color>) Help\n");
            doc_insert(cols[1], "<color:y>ESC</color>) <color:v>Quit</color>\n");
            doc_insert_cols(_doc, cols, 2, 1);
            doc_free(cols[0]);
            doc_free(cols[1]);

            _sync_term(_doc);
            cmd = _inkey();
        }
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '?') doc_display_help("Skillmaster.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < ct)
            {
                _group_ptr g = &_groups[i];
                doc_display_help("Skillmaster.txt", g->name);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < ct)
            {
                _group_ptr g = &_groups[i];
                REPEAT_PUSH(cmd);
                if (_gain_skill_ui(g) == UI_OK) return UI_OK;
                REPEAT_POP();
            }
        }
        cmd = 0;
    }
}

void skillmaster_gain_skill(void)
{
    assert(!_doc);
    _doc = doc_alloc(80);

    msg_line_clear();
    Term_save();
    if (_gain_type_ui() == UI_OK)
    {
        p_ptr->update |= PU_BONUS | PU_HP | PU_MANA;
        p_ptr->redraw |= PR_EFFECTS | PR_HP | PR_MANA;
    }
    Term_load();

    doc_free(_doc);
    _doc = NULL;
}

/************************************************************************
 * Melee Skills
 ***********************************************************************/
static void _melee_init_class(class_t *class_ptr)
{
    typedef struct { int base_thn; int xtra_thn; int str; int dex; } _melee_skill_t;
    static _melee_skill_t _tbl[11] = {
        { 34,  6, 0, 0 },
        { 50, 10, 1, 0 },
        { 55, 12, 1, 1 },
        { 60, 15, 2, 1 },
        { 65, 18, 2, 1 },
        { 70, 21, 2, 1 },

        { 70, 23, 2, 2 },
        { 70, 25, 3, 2 },
        { 70, 27, 3, 2 },
        { 70, 29, 3, 2 },
        { 70, 30, 4, 2 }
    };
    int pts = MIN(10, _get_group_pts(_TYPE_MELEE));
    _melee_skill_t row = _tbl[pts];
    class_ptr->base_skills.thn += row.base_thn;
    class_ptr->extra_skills.thn += row.xtra_thn;
    class_ptr->stats[A_STR] += row.str;
    class_ptr->stats[A_DEX] += row.dex;

    pts = _get_skill_pts(_TYPE_MELEE, _MARTIAL_ARTS);
    class_ptr->stats[A_DEX] += (pts + 1)/3;
}

typedef struct { int to_h; int to_d; int prof; int blows_max; int blows_mult; int ma_wgt; } _melee_info_t;
static _melee_info_t _melee_info[6] = {
    {  0,  0, 2000, 400, 20,   0 },
    {  0,  0, 4000, 500, 30,  60 },
    {  0,  0, 6000, 525, 40,  70 },
    {  5,  0, 7000, 550, 50,  80 },
    { 10,  3, 8000, 575, 55,  90 },
    { 20, 10, 8000, 600, 60, 100 }
};

static void _calc_weapon_bonuses(object_type *o_ptr, weapon_info_t *info_ptr)
{
    int           pts = _get_skill_pts(_TYPE_MELEE, o_ptr->tval);
    _melee_info_t info;

    assert(0 <= pts && pts <= 5);
    info = _melee_info[pts];

    info_ptr->to_h += info.to_h;
    info_ptr->dis_to_h += info.to_h;

    info_ptr->to_d += info.to_d;
    info_ptr->dis_to_d += info.to_d;
}

int skillmaster_get_blows_max(object_type *o_ptr)
{
    int pts = _get_skill_pts(_TYPE_MELEE, o_ptr->tval);
    assert(0 <= pts && pts <= 5);
    return _melee_info[pts].blows_max;
}

int skillmaster_get_blows_mult(object_type *o_ptr)
{
    int pts = _get_skill_pts(_TYPE_MELEE, o_ptr->tval);
    int mult;
    assert(0 <= pts && pts <= 5);
    mult = _melee_info[pts].blows_mult;
    if (p_ptr->riding)
    {
        u32b flgs[OF_ARRAY_SIZE];
        obj_flags(o_ptr, flgs);
        if (have_flag(flgs, OF_RIDING))
        {
            pts = _get_skill_pts(_TYPE_TECHNIQUE, _RIDING);
            mult += 5 * pts;
        }
    }
    return mult;
}

int skillmaster_weapon_prof(int tval)
{
    int pts = _get_skill_pts(_TYPE_MELEE, tval);
    assert(0 <= pts && pts <= 5);
    return _melee_info[pts].prof;
}

int skillmaster_martial_arts_prof(void)
{
    int pts = _get_skill_pts(_TYPE_MELEE, _MARTIAL_ARTS);
    assert(0 <= pts && pts <= 5);
    if (!pts)
        return 0; /* rather than 2000 which would allow the player to attempt bare-handed combat */
    return _melee_info[pts].prof;
}

static void _melee_calc_bonuses(void)
{
    int pts = _get_skill_pts(_TYPE_MELEE, _MARTIAL_ARTS);
    assert(0 <= pts && pts <= 5);
    p_ptr->monk_lvl = (p_ptr->lev * _melee_info[pts].ma_wgt + 50) / 100;
    if (!equip_find_first(object_is_melee_weapon) && p_ptr->monk_lvl)
        monk_ac_bonus();
}

/************************************************************************
 * Shoot Skills
 ***********************************************************************/
static void _shoot_init_class(class_t *class_ptr)
{
    typedef struct { int base_thb; int xtra_thb; } _shoot_skill_t;
    static _shoot_skill_t _tbl[6] = {
        { 20, 11 },
        { 40, 15 },
        { 50, 20 },
        { 55, 25 },
        { 60, 27 },
        { 70, 30 }
    };
    int pts = MIN(5, _get_group_pts(_TYPE_SHOOT));
    _shoot_skill_t row = _tbl[pts];
    class_ptr->base_skills.thb += row.base_thb;
    class_ptr->extra_skills.thb += row.xtra_thb;

    pts = _get_skill_pts(_TYPE_SHOOT, _THROWING);
    class_ptr->stats[A_DEX] += (pts + 1) / 2;

    pts = _get_skill_pts(_TYPE_SHOOT, _ARCHERY);
    class_ptr->base_skills.stl += (pts + 1) / 2;
}

typedef struct { int to_h; int to_d; int prof; int shots; } _shoot_info_t;
static _shoot_info_t _shoot_info[6] = {
    {  0,  0, 2000,   0 },

    {  0,  0, 4000,   0 },
    {  1,  0, 6000,  25 },
    {  3,  0, 7000,  50 },
    {  5,  2, 8000, 100 },
    { 10,  5, 8000, 150 }
};

static void _calc_shooter_bonuses(object_type *o_ptr, shooter_info_t *info_ptr)
{
    if ( !p_ptr->shooter_info.heavy_shoot
      && info_ptr->tval_ammo <= TV_BOLT
      && info_ptr->tval_ammo >= TV_SHOT )
    {
        int pts = _get_skill_pts(_TYPE_SHOOT, _ARCHERY);
        _shoot_info_t row;
        assert(0 <= pts && pts <= 5);
        row = _shoot_info[pts];
        p_ptr->shooter_info.to_h += row.to_h;
        p_ptr->shooter_info.dis_to_h += row.to_h;
        p_ptr->shooter_info.to_d += row.to_d;
        p_ptr->shooter_info.dis_to_d += row.to_d;
        p_ptr->shooter_info.num_fire += row.shots;
    }
}

int skillmaster_bow_prof(void)
{
    int pts = _get_skill_pts(_TYPE_SHOOT, _ARCHERY);
    assert(0 <= pts && pts <= 5);
    return _shoot_info[pts].prof;
}

typedef struct { int skill; int back; int mult; int energy; } _throw_info_t;
static _throw_info_t _throw_info[6] = {
    {   0, 15, 100, 100 },
    {  12, 18, 100, 100 },
    {  28, 21, 150, 100 }, /* 18/220 Dex for 0% fail */
    {  48, 24, 200,  90 }, /* 18/180 Dex for 0% fail */
    {  72, 27, 300,  80 }, /* 18/150 Dex for 0% fail */
    { 100, 30, 400,  60 }, /* 18/110 Dex for 0% fail */
};

static void _shoot_calc_bonuses(void)
{
    int pts = _get_skill_pts(_TYPE_SHOOT, _THROWING);
    assert(0 <= pts && pts <= 5);
    /* Bow skills are a bit low for effective throwing, presumably since
     * the code expects to_h bonuses from both shooter and ammo? Note that
     * skill_tht is already initialized to skills.thb in calc_bonuses() */
    p_ptr->skill_tht += _throw_info[pts].skill;
}

/* THROW WEAPON ... TODO: Try to consolidate this with the 2 other versions in weaponmaster.c */
typedef struct {
    int item;
    object_type *o_ptr;
    int mult; /* scaled by 100 */
    int tdis;
    int tx;
    int ty;
    bool come_back;
    bool fail_catch;
} _throw_weapon_info;

static void _throw_weapon_imp(_throw_weapon_info * info_ptr);

static int _throw_back_chance(void)
{
    int result;
    int pts = _get_skill_pts(_TYPE_SHOOT, _THROWING);
    assert(0 <= pts && pts <= 5);

    result = randint1(30);
    result += _throw_info[pts].back;
    result += adj_dex_th[p_ptr->stat_ind[A_DEX]];
    result -= 128;

    return result;
}

static int _throw_mult(int hand)
{
    int result;
    int pts = _get_skill_pts(_TYPE_SHOOT, _THROWING);
    assert(0 <= pts && pts <= 5);

    result = _throw_info[pts].mult;
    if (p_ptr->mighty_throw)
        result += 100;

    return result;
}

static int _throw_range(int hand)
{
    int          mul, div, rng;
    object_type *o_ptr = equip_obj(p_ptr->weapon_info[hand].slot);

    mul = 10 + 2 * (_throw_mult(hand) - 100) / 100;
    div = o_ptr->weight > 10 ? o_ptr->weight : 10;
    div /= 2;

    rng = (adj_str_blow[p_ptr->stat_ind[A_STR]] + 20) * mul / div;
    if (rng > mul) rng = mul;
    if (rng < 5) rng = 5;

    return rng;
}

static bool _throw_weapon(int hand)
{
    int dir;
    _throw_weapon_info info;
    int back_chance;
    int oops = 100;

    /* Setup info for the toss */
    info.item = p_ptr->weapon_info[hand].slot;
    info.o_ptr = equip_obj(p_ptr->weapon_info[hand].slot);
    info.come_back = FALSE;

    if (!info.o_ptr) return FALSE; /* paranoia */
    if (object_is_cursed(info.o_ptr))
    {
        msg_print("Hmmm, it seems to be cursed.");
        return FALSE;
    }

    back_chance = _throw_back_chance();

    info.come_back = FALSE;
    info.fail_catch = FALSE;
    if (back_chance > 30 && !one_in_(oops))
    {
        info.come_back = TRUE;
        if (p_ptr->blind || p_ptr->image || p_ptr->confused || one_in_(oops))
            info.fail_catch = TRUE;
        else
        {
            oops = 37;
            if (p_ptr->stun)
                oops += 10;
            if (back_chance <= oops)
                info.fail_catch = TRUE;
        }
    }

    /* Pick a target */
    info.mult = _throw_mult(hand);
    info.tdis = _throw_range(hand);

    project_length = info.tdis;
    if (!get_fire_dir(&dir)) return FALSE;

    info.tx = px + 99 * ddx[dir];
    info.ty = py + 99 * ddy[dir];

    if (dir == 5 && target_okay())
    {
        info.tx = target_col;
        info.ty = target_row;
    }
    project_length = 0;

    if (info.tx == px && info.ty == py) return FALSE;

    /* Throw */
    _throw_weapon_imp(&info);

    /* Handle Inventory */
    if (!info.come_back || info.fail_catch)
    {
        object_type copy;

        if (!info.come_back)
        {
            char o_name[MAX_NLEN];
            object_desc(o_name, info.o_ptr, OD_NAME_ONLY);
            msg_format("Your %s fails to return!", o_name);
        }

        if (info.fail_catch)
            cmsg_print(TERM_VIOLET, "But you can't catch!");

        if (TRUE) /* This is a showstopper, so force the player to notice! */
        {
            msg_print("Press <color:y>Space</color> to continue.");
            flush();
            for (;;)
            {
                char ch = inkey();
                if (ch == ' ') break;
            }
            msg_line_clear();
        }

        object_copy(&copy, info.o_ptr);
        copy.number = 1;

        inven_item_increase(info.item, -1);
        inven_item_describe(info.item);
        inven_item_optimize(info.item);


        if (!info.come_back)
            drop_near(&copy, 0, info.ty, info.tx);
        else
            drop_near(&copy, 0, py, px);

        p_ptr->redraw |= PR_EQUIPPY;
        p_ptr->update |= PU_BONUS;
        android_calc_exp();
        handle_stuff();
    }
    return TRUE;
}

static void _throw_weapon_imp(_throw_weapon_info * info)
{
    char o_name[MAX_NLEN];
    u16b path[512];
    int msec = delay_factor * delay_factor * delay_factor;
    int y, x, ny, nx, tdam;
    int cur_dis, ct;
    int chance;

    chance = p_ptr->skill_tht + (p_ptr->shooter_info.to_h + info->o_ptr->to_h) * BTH_PLUS_ADJ;

    object_desc(o_name, info->o_ptr, OD_NAME_ONLY);
    ct = project_path(path, info->tdis, py, px, info->ty, info->tx, PROJECT_PATH);

    y = py;
    x = px;

    for (cur_dis = 0; cur_dis < ct; )
    {
        /* Peek ahead at the next square in the path */
        ny = GRID_Y(path[cur_dis]);
        nx = GRID_X(path[cur_dis]);

        /* Stopped by walls/doors/forest ... but allow hitting your target, please! */
        if (!cave_have_flag_bold(ny, nx, FF_PROJECT)
         && !cave[ny][nx].m_idx)
        {
            break;
        }

        /* The player can see the (on screen) missile */
        if (panel_contains(ny, nx) && player_can_see_bold(ny, nx))
        {
            char c = object_char(info->o_ptr);
            byte a = object_attr(info->o_ptr);

            /* Draw, Hilite, Fresh, Pause, Erase */
            print_rel(c, a, ny, nx);
            move_cursor_relative(ny, nx);
            Term_fresh();
            Term_xtra(TERM_XTRA_DELAY, msec);
            lite_spot(ny, nx);
            Term_fresh();
        }

        /* The player cannot see the missile */
        else
        {
            /* Pause anyway, for consistancy */
            Term_xtra(TERM_XTRA_DELAY, msec);
        }

        /* Save the new location */
        x = nx;
        y = ny;

        /* Advance the distance */
        cur_dis++;

        /* Monster here, Try to hit it */
        if (cave[y][x].m_idx)
        {
            cave_type *c_ptr = &cave[y][x];
            monster_type *m_ptr = &m_list[c_ptr->m_idx];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];
            bool visible = m_ptr->ml;

            if (test_hit_fire(chance - cur_dis, MON_AC(r_ptr, m_ptr), m_ptr->ml))
            {
                bool fear = FALSE;

                if (!visible)
                    msg_format("The %s finds a mark.", o_name);
                else
                {
                    char m_name[80];
                    monster_desc(m_name, m_ptr, 0);
                    msg_format("The %s hits %s.", o_name, m_name);
                    if (m_ptr->ml)
                    {
                        if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
                        health_track(c_ptr->m_idx);
                    }
                }

                /***** The Damage Calculation!!! *****/
                tdam = damroll(info->o_ptr->dd, info->o_ptr->ds);
                tdam = tot_dam_aux(info->o_ptr, tdam, m_ptr, 0, 0, TRUE);
                tdam = critical_throw(info->o_ptr->weight, info->o_ptr->to_h, tdam);
                tdam += info->o_ptr->to_d;
                tdam += adj_str_td[p_ptr->stat_ind[A_STR]] - 128;
                tdam = tdam * info->mult / 100;
                /*tdam += p_ptr->shooter_info.to_d; <== It feels wrong for Rings of Archery to work here!*/
                if (tdam < 0) tdam = 0;
                tdam = mon_damage_mod(m_ptr, tdam, FALSE);

                if (mon_take_hit(c_ptr->m_idx, tdam, &fear, extract_note_dies(real_r_ptr(m_ptr))))
                {
                    /* Dead monster */
                }
                else
                {
                    message_pain(c_ptr->m_idx, tdam);
                    if (tdam > 0)
                        anger_monster(m_ptr);

                    if (tdam > 0 && m_ptr->cdis > 1 && allow_ticked_off(r_ptr))
                    {
                        if (mut_present(MUT_PEERLESS_SNIPER))
                        {
                        }
                        else
                        {
                            m_ptr->anger_ct++;
                        }
                    }

                    if (fear && m_ptr->ml)
                    {
                        char m_name[80];
                        sound(SOUND_FLEE);
                        monster_desc(m_name, m_ptr, 0);
                        msg_format("%^s flees in terror!", m_name);
                    }
                }
            }

            /* Stop looking */
            break;
        }
    }

    if (info->come_back)
    {
        int i;
        for (i = cur_dis; i >= 0; i--)
        {
            y = GRID_Y(path[i]);
            x = GRID_X(path[i]);
            if (panel_contains(y, x) && player_can_see_bold(y, x))
            {
                char c = object_char(info->o_ptr);
                byte a = object_attr(info->o_ptr);

                /* Draw, Hilite, Fresh, Pause, Erase */
                print_rel(c, a, y, x);
                move_cursor_relative(y, x);
                Term_fresh();
                Term_xtra(TERM_XTRA_DELAY, msec);
                lite_spot(y, x);
                Term_fresh();
            }
            else
            {
                /* Pause anyway, for consistancy */
                Term_xtra(TERM_XTRA_DELAY, msec);
            }
        }
        msg_format("Your %s comes back to you.", o_name);
    }
    else
    {
        /* Record the actual location of the toss so we can drop the object here if required */
        info->tx = x;
        info->ty = y;
    }
}

static int _throwing_hand(void)
{
    int hand;
    for (hand = 0; hand < MAX_HANDS; hand++)
    {
        if (p_ptr->weapon_info[hand].wield_how != WIELD_NONE && !p_ptr->weapon_info[hand].bare_hands)
            return hand;
    }
    return HAND_NONE;
}

static void _throw_weapon_spell(int cmd, variant *res)
{
    switch (cmd)
    {
    case SPELL_NAME:
        var_set_string(res, "Throw Weapon");
        break;
    case SPELL_DESC:
        var_set_string(res, "Throws your leading weapon, which might return to you.");
        break;
    case SPELL_INFO:
    {
        int hand = _throwing_hand();
        if (hand != HAND_NONE)
            var_set_string(res, info_range(_throw_range(hand)));
        break;
    }
    case SPELL_CAST:
    {
        int hand = _throwing_hand();
        var_set_bool(res, FALSE);
        if (hand != HAND_NONE)
            var_set_bool(res, _throw_weapon(hand));
        else
            msg_print("You need to wield a weapon before throwing it.");
        break;
    }
    case SPELL_ENERGY:
    {
        int pts = _get_skill_pts(_TYPE_SHOOT, _THROWING);
        assert(0 <= pts && pts <= 5);
        var_set_int(res, _throw_info[pts].energy);
        break;
    }
    default:
        default_spell(cmd, res);
        break;
    }
}

/************************************************************************
 * Magic/Prayer Skills
 ***********************************************************************/
static void _magic_init_class(class_t *class_ptr)
{
    typedef struct { int base_dev; int xtra_dev; int int_; int str; int con; } _magic_skill_t;
    static _magic_skill_t _tbl[11] = {
        { 23,  9, 0,  0,  0 },

        { 25,  9, 1,  0,  0 },
        { 27,  9, 1,  0,  0 },
        { 29,  9, 2, -1,  0 },
        { 30,  9, 2, -1,  0 },
        { 31,  9, 3, -1, -1 },

        { 32, 10, 3, -2, -1 },
        { 33, 10, 3, -2, -1 },
        { 34, 10, 3, -2, -1 },
        { 35, 10, 3, -2, -2 },
        { 35, 11, 4, -3, -2 }
    };
    int pts = _get_group_pts(_TYPE_MAGIC);
    _magic_skill_t row = _tbl[MIN(10, pts)];
    class_ptr->base_skills.dev += row.base_dev;
    class_ptr->extra_skills.dev += row.xtra_dev;
    class_ptr->stats[A_INT] += row.int_;
    class_ptr->stats[A_STR] += row.str;
    class_ptr->stats[A_CON] += row.con;
    class_ptr->life -= (pts + 1) / 2;
}

static void _prayer_init_class(class_t *class_ptr)
{
    typedef struct { int base_sav; int xtra_sav; int wis; } _prayer_skill_t;
    static _prayer_skill_t _tbl[11] = {
        { 31, 10, 0 },

        { 32, 11, 1 },
        { 33, 11, 1 },
        { 34, 11, 2 },
        { 35, 11, 2 },
        { 36, 11, 3 },

        { 38, 11, 3 },
        { 39, 12, 4 },
        { 40, 12, 4 },
        { 40, 13, 4 },
        { 40, 14, 4 }
    };
    int pts = _get_group_pts(_TYPE_PRAYER);
    _prayer_skill_t row = _tbl[MIN(10, pts)];
    class_ptr->base_skills.sav += row.base_sav;
    class_ptr->extra_skills.sav += row.xtra_sav;
    class_ptr->stats[A_WIS] += row.wis;
}

typedef struct { int max_lvl; int cost_mult; int fail_adj; int fail_min; } _realm_skill_t;
static _realm_skill_t _realm_skills[6] = {
    {  1,   0,  0, 50 },
    { 30, 150, 10,  5 },
    { 35, 125,  5,  5 },
    { 40, 110,  2,  4 },
    { 45, 100,  1,  3 },
    { 50,  90,  0,  0 }
};

caster_info *_caster_info(void)
{
    static caster_info info = {0};
    int magic_pts = _get_group_pts(_TYPE_MAGIC);
    int prayer_pts = _get_group_pts(_TYPE_PRAYER);

    info.options = 0;
    info.weight = 450;
    /* Experimental: Learning Kendo let's you focus
     * to supercharge mana, no matter what other realms you
     * know. This is powerful, so we preclude access
     * to CASTER_ALLOW_DEC_MANA altogether. Also, unlike
     * Samurai, you still need to carry books! */
    if (_get_skill_pts(_TYPE_TECHNIQUE, REALM_HISSATSU))
    {
        info.which_stat = A_WIS;
        info.magic_desc = "technique";
        info.options = CASTER_SUPERCHARGE_MANA;
        return &info;
    }
    /* Now, use INT or WIS for mana, depending on which
     * has the most total points */
    else if (magic_pts && magic_pts > prayer_pts)
    {
        info.which_stat = A_INT;
        info.magic_desc = "spell";
        info.options |= CASTER_GLOVE_ENCUMBRANCE;
        if (magic_pts >= 5)
            info.options |= CASTER_ALLOW_DEC_MANA;
        return &info;
    }
    else if (prayer_pts)
    {
        info.which_stat = A_WIS;
        info.magic_desc = "prayer";
        if (prayer_pts >= 5)
            info.options |= CASTER_ALLOW_DEC_MANA;
        return &info;
    }
    else if (_get_skill_pts(_TYPE_TECHNIQUE, REALM_BURGLARY))
    {
        info.which_stat = A_DEX;
        info.magic_desc = "thieving talent";
        return &info;
    }
    return NULL;
}

/************************************************************************
 * Spellcasting UI
 ***********************************************************************/
typedef struct {
    int level;
    int cost;
    int fail;
    int realm;
    int idx;
} _spell_info_t, *_spell_info_ptr;

static bool _has_magic(void)
{
    if ( !_get_group_pts(_TYPE_MAGIC)
      && !_get_group_pts(_TYPE_PRAYER)
      && !_get_skill_pts(_TYPE_TECHNIQUE, REALM_BURGLARY)
      && !_get_skill_pts(_TYPE_TECHNIQUE, REALM_HISSATSU) )
    {
        return FALSE;
    }
    return TRUE;
}

static bool _can_cast(void)
{
    if (!_has_magic())
    {
        msg_print("You don't know any spell realms. Use the 'G' command to gain the appropriate skills.");
        flush();
        return FALSE;
    }
    if (p_ptr->blind || no_lite())
    {
        msg_print("You cannot see!");
        flush();
        return FALSE;
    }
    if (p_ptr->confused)
    {
        msg_print("You are too confused!");
        flush();
        return FALSE;
    }
    return TRUE;
}

static int _get_realm_pts(int realm)
{
    _skill_ptr s;
    if (realm == REALM_BURGLARY || realm == REALM_HISSATSU)
    {
        s = _get_skill(_TYPE_TECHNIQUE, realm);
        assert(s);
        return s->current;
    }
    /* Assert: A realm, if known, is only known in either Magic or Prayer
     * but never in both */
    s = _get_skill(_TYPE_MAGIC, realm);
    if (s && s->current)
        return s->current;
    s = _get_skill(_TYPE_PRAYER, realm);
    if (s && s->current)
        return s->current;
    return 0;
}

static int _get_realm_stat(int realm)
{
    _skill_ptr s;
    if (realm == REALM_BURGLARY)
        return A_DEX;
    else if (realm == REALM_HISSATSU)
        return A_WIS;
    s = _get_skill(_TYPE_MAGIC, realm);
    if (s && s->current)
        return A_INT;
    s = _get_skill(_TYPE_PRAYER, realm);
    if (s && s->current)
        return A_WIS;
    return A_NONE;
}

static int _get_realm_lvl(int realm)
{
    int pts = _get_realm_pts(realm);
    int max = 0;

    assert(0 <= pts && pts <= 5);
    max = _realm_skills[pts].max_lvl;
    if (p_ptr->lev > max)
        return max;
    return p_ptr->lev;
}

int py_casting_lvl(int realm)
{
    if (p_ptr->pclass != CLASS_SKILLMASTER)
        return p_ptr->lev;
    return _get_realm_lvl(realm);
}

static bool _spellbook_hook(object_type *o_ptr)
{
    if (TV_BOOK_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_BOOK_END)
    {
        int realm = tval2realm(o_ptr->tval);
        int pts = _get_realm_pts(realm);
        if (pts > 0)
            return TRUE;
    }
    return FALSE;
}

/* Note: At the moment, we only support spellbook base realms. Should that
 * change, I will probably need to replace get_item() with something more
 * intelligent that also handles non-book based realms in a single menu. */
static object_type *_prompt_spellbook(void)
{
    int item;
    item_tester_hook = _spellbook_hook;
    if (!get_item(&item, "Use which book? ", "You have no spellbooks!", USE_INVEN | USE_FLOOR))
        return NULL;
    if (item >= 0)
        return &inventory[item];
    else
        return &o_list[-item];
}

static vec_ptr _get_spell_list(object_type *spellbook)
{
    vec_ptr        v = vec_alloc(free);
    int            realm = tval2realm(spellbook->tval);
    int            pts = _get_realm_pts(realm);
    _realm_skill_t skill;
    int            start = 8*spellbook->sval;
    int            stop = start + 8;
    int            lvl = _get_realm_lvl(realm);
    int            stat = p_ptr->stat_ind[_get_realm_stat(realm)];
    int            i;

    assert(0 < pts && pts <= 5);
    skill = _realm_skills[pts];
    for (i = start; i < stop; i++)
    {
        magic_type      info;
        _spell_info_ptr spell = malloc(sizeof(_spell_info_t));

        if (is_magic(realm))
            info = mp_ptr->info[realm - 1][i];
        else
            info = technic_info[realm - MIN_TECHNIC][i];
        spell->level = info.slevel;
        spell->cost = info.smana * skill.cost_mult / 100;
        /* Dec mana: The realm stones always work, but wizardstaves et. al.
         * require both 5 pts total in Magic/Prayer and 4 pts in the realm
         * in question */
        if (spell->cost && is_magic(realm))
        {
            if (p_ptr->easy_realm1 == realm || (p_ptr->dec_mana && pts >= 4))
                spell->cost = MAX(1, spell->cost * 3 / 4);
        }
        if (realm == REALM_HISSATSU)
            spell->fail = 0;
        else
        {
            spell->fail = calculate_fail_rate_aux(lvl, info.slevel, MIN(100, MAX(0, info.sfail + skill.fail_adj)), stat);
            if (spell->fail < skill.fail_min)
                spell->fail = skill.fail_min;
        }
        spell->realm = realm;
        spell->idx = i;
        vec_add(v, spell);
    }
    return v;
}

/* This is shared with the character dump ... */
static void _list_spells(doc_ptr doc, object_type *spellbook, vec_ptr spells, int browse_idx)
{
    int          i;
    object_kind *k_ptr = &k_info[spellbook->k_idx];
    int          realm = tval2realm(spellbook->tval);
    int          lvl = _get_realm_lvl(realm);

    doc_printf(doc, "<color:%c>%-27.27s</color> <color:G>Lvl  SP Fail Desc</color>\n",
        attr_to_attr_char(k_ptr->d_attr), k_name + k_ptr->name);
    
    for (i = 0; i < vec_length(spells); i++)
    {
        _spell_info_ptr spell = vec_get(spells, i);
        if (spell->level >= 99)
        {
            doc_printf(doc, " <color:D>%c) Illegible</color>\n", I2A(i));
        }
        else
        {
            doc_printf(doc, " <color:%c>%c</color>) ",
                (spell->level <= lvl && spell->cost <= p_ptr->csp) ? 'y' : 'D', I2A(i));
            doc_printf(doc, "<color:%c>%-23.23s</color> ",
                i == browse_idx ? 'B' : 'w',
                do_spell(spell->realm, spell->idx, SPELL_NAME));
            doc_printf(doc, "%3d <color:%c>%3d</color> %3d%% ",
                spell->level,
                spell->cost <= p_ptr->csp ? 'w' : 'r',
                spell->cost, spell->fail);
            if (spell->level <= lvl)
                doc_printf(doc, "%s", do_spell(spell->realm, spell->idx, SPELL_INFO));
            doc_newline(doc);
        }
    }
}

#define _BROWSE_MODE 0x01
static bool _prompt_spell(object_type *spellbook, _spell_info_ptr chosen_spell, int options)
{
    bool    result = FALSE;
    vec_ptr spells = _get_spell_list(spellbook);
    int     browse_idx = -1, cmd, i;
    int     realm = tval2realm(spellbook->tval);
    int     lvl = _get_realm_lvl(realm);

    if (REPEAT_PULL(&cmd))
    {
        i = A2I(cmd);
        if (0 <= i && i < vec_length(spells))
        {
            _spell_info_ptr spell = vec_get(spells, i);
            if (spell->level <= lvl && spell->cost <= p_ptr->csp)
            {
                *chosen_spell = *spell;
                vec_free(spells);
                return TRUE;
            }
        }
    }

    assert(!_doc);
    _doc = doc_alloc(72);

    msg_line_clear();
    Term_save();
    for (;;)
    {
        doc_clear(_doc);
        _list_spells(_doc, spellbook, spells, browse_idx);
        if (0 <= browse_idx && browse_idx < vec_length(spells))
        {
            _spell_info_ptr spell = vec_get(spells, browse_idx);
            doc_newline(_doc);
            doc_insert(_doc, do_spell(spell->realm, spell->idx, SPELL_DESC));
            doc_newline(_doc);
        }
        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) break;
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < vec_length(spells))
                browse_idx = i;
        }
        else if (islower(cmd))
        {
            i = A2I(cmd);
            if (0 <= i && i < vec_length(spells))
            {
                if (options & _BROWSE_MODE)
                    browse_idx = i;
                else
                {
                    _spell_info_ptr spell = vec_get(spells, i);
                    if (spell->level <= lvl && spell->cost <= p_ptr->csp)
                    {
                        *chosen_spell = *spell;
                        result = TRUE;
                        REPEAT_PUSH(cmd);
                        break;
                    }
                }
            }
        }
    }
    Term_load();

    vec_free(spells);
    doc_free(_doc);
    _doc = NULL;

    return result;
}

static void _cast_spell(_spell_info_ptr spell)
{
    int lvl = _get_realm_lvl(spell->realm);

    assert(spell->level <= lvl);
    assert(spell->cost <= p_ptr->csp);

    p_ptr->csp -= spell->cost;
    energy_use = 100;

    if (randint0(100) < spell->fail)
    {
        if (flush_failure) flush();

        cmsg_format(TERM_VIOLET, "You failed to cast <color:B>%s</color>!", do_spell(spell->realm, spell->idx, SPELL_NAME));
        if (demigod_is_(DEMIGOD_ATHENA))
            p_ptr->csp += spell->cost/2;
        spell_stats_on_fail_old(spell->realm, spell->idx);
        sound(SOUND_FAIL);
        do_spell(spell->realm, spell->idx, SPELL_FAIL);
    }
    else
    {
        if (!do_spell(spell->realm, spell->idx, SPELL_CAST))
        {  /* Canceled */
            p_ptr->csp += spell->cost;
            energy_use = 0;
            return;
        }
        sound(SOUND_ZAP);
        spell_stats_on_cast_old(spell->realm, spell->idx);
    }
    p_ptr->redraw |= PR_MANA;
    p_ptr->window |= PW_SPELL;
}

void skillmaster_cast(void)
{
    if (_can_cast())
    {
        object_type  *spellbook = _prompt_spellbook();
        _spell_info_t spell = {0};
        
        if (!spellbook)
            return;
        if (spellbook->tval == TV_HISSATSU_BOOK && !equip_find_first(object_is_melee_weapon))
        {
            if (flush_failure) flush();
            msg_print("You need to wield a weapon!");
            return;
        }
        if (_prompt_spell(spellbook, &spell, 0))
            _cast_spell(&spell);
    }
}

void skillmaster_browse(void)
{
    if (_can_cast())
    {
        object_type  *spellbook = _prompt_spellbook();
        _spell_info_t spell = {0};
        
        if (!spellbook)
            return;
        _prompt_spell(spellbook, &spell, _BROWSE_MODE);
    }
}

/************************************************************************
 * Skills
 ***********************************************************************/
static void _skills_init_class(class_t *class_ptr)
{
    int pts;

    pts = _get_skill_pts(_TYPE_SKILLS, _AGILITY);
    class_ptr->stats[A_DEX] += pts;
    class_ptr->base_skills.dis += 25 + 7*pts;
    class_ptr->extra_skills.dis += 7 + 2*pts;

    pts = _get_skill_pts(_TYPE_SKILLS, _AWARENESS);
    class_ptr->base_skills.srh += 12 + 15*pts;
    class_ptr->base_skills.fos += 6 + 15*pts;
    if (!pts)
        class_ptr->flags = CLASS_SENSE1_MED | CLASS_SENSE1_WEAK;
    else
    {
        class_ptr->flags = CLASS_SENSE1_FAST | CLASS_SENSE1_STRONG |
                           CLASS_SENSE2_FAST | CLASS_SENSE2_STRONG;
    }

    pts = _get_skill_pts(_TYPE_SKILLS, _DEVICES);
    class_ptr->base_skills.dev += 10*pts;

    pts = _get_skill_pts(_TYPE_SKILLS, _HEALTH);
    class_ptr->stats[A_CON] += pts;
    class_ptr->life += 5*pts;
    class_ptr->base_hp += 10*pts;

    pts = _get_skill_pts(_TYPE_SKILLS, _MAGIC_RESISTANCE);
    class_ptr->base_skills.sav += 15*pts;

    pts = _get_skill_pts(_TYPE_SKILLS, _STEALTH);
    class_ptr->base_skills.stl += 3*pts;
}

static void _skills_calc_bonuses(void)
{
    int pts;
    pts = _get_skill_pts(_TYPE_SKILLS, _AWARENESS);
    switch (pts)
    {
    case 3: p_ptr->telepathy = TRUE;
    case 2: p_ptr->auto_pseudo_id = TRUE;
    case 1: p_ptr->see_inv = TRUE;
    }

    pts = _get_skill_pts(_TYPE_SKILLS, _SPEED);
    p_ptr->pspeed += 2*pts;

    pts = _get_skill_pts(_TYPE_SKILLS, _STEALTH);
    if (pts >= 3)
        p_ptr->ambush = TRUE;
}

void _skills_get_flags(u32b flgs[OF_ARRAY_SIZE])
{
    int pts;
    pts = _get_skill_pts(_TYPE_SKILLS, _AWARENESS);
    switch (pts)
    {
    case 3: add_flag(flgs, OF_TELEPATHY);
    case 2: add_flag(flgs, OF_LORE1);
    case 1: add_flag(flgs, OF_SEE_INVIS);
    }

    pts = _get_skill_pts(_TYPE_SKILLS, _SPEED);
    if (pts > 0)
        add_flag(flgs, OF_SPEED);
}

/************************************************************************
 * Techniques
 ***********************************************************************/
void _tech_init_class(class_t *class_ptr)
{
    int pts;

    pts = _get_skill_pts(_TYPE_TECHNIQUE, REALM_BURGLARY);
    class_ptr->stats[A_DEX] += (pts + 2) / 3;
    class_ptr->base_skills.stl += (pts + 2) / 3;
    class_ptr->base_skills.dis += 5*pts;

    pts = _get_skill_pts(_TYPE_TECHNIQUE, REALM_HISSATSU);
    if (pts)
        class_ptr->stats[A_WIS] += 1;
}

void _tech_calc_bonuses(void)
{
    int pts;

    pts = _get_skill_pts(_TYPE_TECHNIQUE, _DUAL_WIELDING);
    if (pts >= 5)
    {
        p_ptr->weapon_info[0].genji = TRUE;
        p_ptr->weapon_info[1].genji = TRUE;
    }
}

int skillmaster_riding_prof(void)
{
    int pts = _get_skill_pts(_TYPE_TECHNIQUE, _RIDING);
    int prof[6] = { 0, RIDING_EXP_BEGINNER, RIDING_EXP_SKILLED, RIDING_EXP_EXPERT, 6500, RIDING_EXP_MASTER };
    return prof[MIN(5, pts)];
}

int skillmaster_dual_wielding_prof(void)
{
    int pts = _get_skill_pts(_TYPE_TECHNIQUE, _DUAL_WIELDING);
    int prof[6] = { 0, WEAPON_EXP_BEGINNER, WEAPON_EXP_SKILLED, WEAPON_EXP_EXPERT, WEAPON_EXP_MASTER, WEAPON_EXP_MASTER };
    return prof[MIN(5, pts)];
}

/************************************************************************
 * Class
 ***********************************************************************/
static void _birth(void)
{
    _reset_groups();
    p_ptr->au += 500; /* build your own class, so buy your own gear! */
}

static void _gain_level(int new_lvl)
{
    if (new_lvl % 5 == 0)
        p_ptr->redraw |= PR_EFFECTS;
}

static void _calc_bonuses(void)
{
    _melee_calc_bonuses();
    _shoot_calc_bonuses();
    _skills_calc_bonuses();
    _tech_calc_bonuses();

    if (_get_skill_pts(_TYPE_ABILITY, _LOREMASTER))
        p_ptr->auto_id = TRUE;
    if (_get_skill_pts(_TYPE_ABILITY, _LUCK))
        p_ptr->good_luck = TRUE;
    if (_get_skill_pts(_TYPE_ABILITY, _REGENERATION))
        p_ptr->regen += 150;
    if (_get_skill_pts(_TYPE_ABILITY, _CLEAR_MIND))
        p_ptr->clear_mind = TRUE;
}

void _get_flags(u32b flgs[OF_ARRAY_SIZE])
{
    _skills_get_flags(flgs);
}

static void _add_power(spell_info* spell, ang_spell fn)
{
    /* Powers are generally granted without cost, or fail. They are available
     * immediately upon purchase, and there is no relevant stat to consider */
    spell->level = 1;
    spell->cost = 0;
    spell->fail = 0;
    spell->fn = fn;
}

static int _get_powers(spell_info* spells, int max)
{
    int ct = 0;

    if (ct < max && _get_skill_pts(_TYPE_SHOOT, _THROWING) > 0)
        _add_power(&spells[ct++], _throw_weapon_spell);
    if (ct < max && _get_skill_pts(_TYPE_TECHNIQUE, REALM_HISSATSU) > 0)
        _add_power(&spells[ct++], samurai_concentration_spell);
    if (ct < max && _get_skill_pts(_TYPE_ABILITY, _CLEAR_MIND) > 0)
        _add_power(&spells[ct++], clear_mind_spell);
    if (ct < max && _get_skill_pts(_TYPE_ABILITY, _EAT_MAGIC) > 0)
        _add_power(&spells[ct++], eat_magic_spell);
    if (ct < max && _get_skill_pts(_TYPE_ABILITY, _MASSACRE) > 0)
        _add_power(&spells[ct++], massacre_spell);
    if (ct < max && _get_skill_pts(_TYPE_ABILITY, _PANIC_HIT) > 0)
        _add_power(&spells[ct++], panic_hit_spell);
    if (ct < max && _get_skill_pts(_TYPE_ABILITY, _RESISTANCE) > 0)
        _add_power(&spells[ct++], resistance_spell);
    if (ct < max && _get_skill_pts(_TYPE_ABILITY, _STONE_SKIN) > 0)
        _add_power(&spells[ct++], stone_skin_spell);

    return ct;
}

static void _dump_book(doc_ptr doc, object_type *spellbook)
{
    vec_ptr spells = _get_spell_list(spellbook);
    _list_spells(doc, spellbook, spells, -1);
    doc_newline(doc);
    vec_free(spells);
}

static object_type *_find_book(int realm, int book)
{
    int tval = realm2tval(realm);
    int sval = book;
    int i;

    for (i = 0; i < INVEN_PACK; i++)
    {
        if (inventory[i].tval == tval && inventory[i].sval == sval)
            return &inventory[i];
    }
    return NULL;
}

static void _dump_realm(doc_ptr doc, int realm)
{
    int i;
    bool first = TRUE;
    for (i = 0; i < 4; i++)
    {
        object_type *spellbook = _find_book(realm, i);
        if (spellbook)
        {
            if (first)
            {
                doc_printf(doc, "<color:r>Realm:</color> <color:B>%s</color>\n\n", realm_names[realm]);
                first = FALSE;
            }
            _dump_book(doc, spellbook);
        }
    }
}

static void _dump_group(doc_ptr doc, _group_ptr g)
{
    int pts = _get_group_pts_aux(g);
    if (pts > 0)
    {
        int i;
        doc_printf(doc, "<color:B>%s</color> (%d)\n", g->name, pts);
        for (i = 0; ; i++)
        {
            _skill_ptr s = &g->skills[i];
            if (!s->type) break;
            if (!s->current) continue;
            doc_printf(doc, "  %s", s->name);
            if (s->max)
                doc_printf(doc, " (%d%%)\n", s->current * 100 / s->max);
            else
                doc_printf(doc, " (%d)\n", s->current);
        }
        doc_newline(doc);
    }
}

static void _character_dump(doc_ptr doc)
{
    int i;
    doc_printf(doc, "<topic:Skills>==================================== <color:keypress>S</color>kills ===================================\n\n");
    for (i = 0; ; i++)
    {
        _group_ptr g = &_groups[i];
        if (!g->type) break;
        _dump_group(doc, g);
    }

    if (_has_magic())
    {
        doc_printf(doc, "<topic:Spells>==================================== <color:keypress>S</color>pells ===================================\n\n");
        for (i = REALM_LIFE; i <= MAX_REALM; i++)
        {
            int pts = _get_realm_pts(i);
            if (pts > 0)
                _dump_realm(doc, i);
        }
    }
}

class_t *skillmaster_get_class(void)
{
    static class_t me = {0};
    static bool init = FALSE;
    int i;

    if (!init)
    {
        me.name = "Skillmaster";
        me.desc = "The Skillmaster is not your ordinary class. Instead, you "
                  "may design your own class, on the fly, using a point based "
                  "skill system. Upon birth, you will get 5 points to spend "
                  "and you should use these wisely to set the basic direction "
                  "of your class. Upon gaining every fifth level, you will "
                  "receive an additional point to spend, for fifteen points "
                  "overall. You may use these points to learn melee or to "
                  "gain access to spell realms. You can improve your speed or "
                  "your stealth. Or you may gain increased magic device skills. "
                  "In addition, you may learn riding skills, dual wielding or "
                  "even martial arts. You will have a lot of flexibility in "
                  "how you choose to play this class.\n \n"
                  "Most skills allow the investment of multiple points for "
                  "increased proficiency, but some are abilities that you may "
                  "buy with a single point (e.g. Luck). This class is not recommended for "
                  "beginning or immediate players. You only have a limited "
                  "amount of points to spend, and your choices are irreversible.";

        me.exp = 130;

        me.birth = _birth;
        me.character_dump = _character_dump;
        me.caster_info = _caster_info;
        me.gain_level = _gain_level;
        me.calc_bonuses = _calc_bonuses;
        me.calc_weapon_bonuses = _calc_weapon_bonuses;
        me.calc_shooter_bonuses = _calc_shooter_bonuses;
        me.get_powers = _get_powers;
        me.get_flags = _get_flags;
        me.load_player = _load_player;
        me.save_player = _save_player;
        init = TRUE;
    }
    /* Reset Stats and Skills on Each Call. Having these things
     * calculated here, rather than in calc_bonuses, enhances the
     * flavor of "building your own class" */
    for (i = 0; i < MAX_STATS; i++)
        me.stats[i] = 0;

    skills_init(&me.base_skills);
    skills_init(&me.extra_skills);

    me.life = 100;
    me.base_hp = 10;
    me.pets = 40;
    me.flags = 0;

    /* Rebuild the class_t, using the current skill allocation */
    if (!spoiler_hack && !birth_hack)
    {
        _melee_init_class(&me);
        _shoot_init_class(&me);
        _magic_init_class(&me);
        _prayer_init_class(&me);
        _skills_init_class(&me);
        _tech_init_class(&me);
    }

    return &me;
}
