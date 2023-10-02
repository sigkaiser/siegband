#include "angband.h"

/****************************************************************
 * Hyperborean
 ****************************************************************/

static void _hyperborean_gain_level(int new_level)
{
    if ((new_level >= 30) && (p_ptr->prace != RACE_DOPPELGANGER)) 
      /* bostock says doppel barbies are strong enough without a demigod power */
    {
        if (p_ptr->demigod_power[0] < 0)
        {
            int idx = mut_gain_choice(mut_demigod_pred/*mut_human_pred*/);
            mut_lock(idx);
            p_ptr->demigod_power[0] = idx;
        }
        else if (!mut_present(p_ptr->demigod_power[0]))
        {
            mut_gain(p_ptr->demigod_power[0]);
            mut_lock(p_ptr->demigod_power[0]);
        }
    }
}

static power_info _hyperborean_get_powers[] =
{
    { A_STR, {10, 5, 30, berserk_spell}},
    { A_WIS, {20, 10, 50, stone_skin_spell}},
    { A_WIS, { 50, 50, 80, wraithform_spell}},

    { -1, {-1, -1, -1, NULL} }
};
static void _hyperborean_calc_bonuses(void)
{
    if (p_ptr->lev >= 5)
    {
        res_add(RES_FEAR);
        p_ptr->pspeed += 1;
    }

    if (p_ptr->lev >= 10)
    {
        p_ptr->see_inv++;
        p_ptr->regen += 100;
        p_ptr->pspeed += 1;
    }

    if (p_ptr->lev >= 15)
    {
        p_ptr->sustain_wis = TRUE;
        p_ptr->sustain_con = TRUE;
        p_ptr->sustain_chr = TRUE;        
        p_ptr->pspeed += 1;
        p_ptr->free_act++;
    } 
    if (p_ptr->lev >= 20)
    { 
        res_add(RES_NETHER);
        res_add(RES_COLD);
        p_ptr->pspeed += 1;
    }
    if (p_ptr->lev >= 25)
    {
        p_ptr->sh_cold++;
        p_ptr->pspeed += 1;
    }
    if (p_ptr->lev >= 30)
    {
        p_ptr->hold_life++;
        res_add(RES_TIME);
        p_ptr->pspeed += 1;
    } 
    if (p_ptr->lev >= 35)
    {
        res_add(RES_DARK);
        p_ptr->pspeed += 1;
    }    
    if (p_ptr->lev >= 40)
    {
        p_ptr->pspeed += 3;
        res_add(RES_TELEPORT);
        res_add(RES_CONF);
    }

    if (p_ptr->lev >= 45) p_ptr->magic_resistance = 50;

}
static void _hyperborean_get_flags(u32b flgs[OF_ARRAY_SIZE])
{
    if (p_ptr->lev >= 5)
    {
        add_flag(flgs, OF_RES_FEAR);
        add_flag(flgs, OF_SPEED);
    }


    if (p_ptr->lev >= 10)
    {
        add_flag(flgs, OF_SEE_INVIS);
        add_flag(flgs, OF_REGEN);
    }
    if (p_ptr->lev >= 15)
    {
        add_flag(flgs, OF_SUST_WIS);
        add_flag(flgs, OF_SUST_CON);
        add_flag(flgs, OF_SUST_STR);
        add_flag(flgs, OF_FREE_ACT);
    }

    if (p_ptr->lev >= 20)
    {
        add_flag(flgs, OF_RES_NETHER);
        add_flag(flgs, OF_RES_COLD);
    }

    if (p_ptr->lev >= 25)
    {
        add_flag(flgs, OF_AURA_COLD);
    }

    if (p_ptr->lev >= 30)
    {
        add_flag(flgs, OF_RES_TIME);
        add_flag(flgs, OF_HOLD_LIFE);
    }
    if (p_ptr->lev >= 35)
    {
        add_flag(flgs, OF_RES_DARK);
    }

    if (p_ptr->lev >= 40)
        add_flag(flgs, OF_RES_CONF);

    if (p_ptr->lev >= 45) 
        add_flag(flgs, OF_MAGIC_RESISTANCE);

}
race_t *hyperborean_get_race(void)
{
    static race_t me = {0};
    static bool init = FALSE;

    if (!init)
    {
        me.name = "Hyperborean";
        me.desc = "The Hyperboreans are an ancient race of demigods posessed of a Spirit"
                  "whose Origin exists outside of creation. Adherents to the Blood Pact,"
                  "they are ferocious warriors and are immune to fear. Having returned from"
                  "the infinite blackness of the Kalibur death, they are able to dominate "
                  "their corporeal form through the willful application of the Selbst,"
                  "allowing them a firm hold on their life force and a resistance to cold," 
                  "nether, and time. They can also perceive invisible things with the eyes "
                  "of the Spirit. These powers will slowly reveal themselves as the Virya"
                  "regains his strategic orientation, eventually enabling him to become a"
                  "mighty Siddha... ";

        me.stats[A_STR] =  2;
        me.stats[A_INT] =  1;
        me.stats[A_WIS] =  3;
        me.stats[A_DEX] =  1;
        me.stats[A_CON] =  2;
        me.stats[A_CHR] =  1;

        me.skills.dis =  4;
        me.skills.dev =  3;
        me.skills.sav =  3;
        me.skills.stl =  2;
        me.skills.srh =  3;
        me.skills.fos = 13;
        me.skills.thn = 15;
        me.skills.thb =  7;

        me.life = 103;
        me.base_hp = 20;
        me.exp = 175;
        me.infra = 0;
        me.shop_adjust = 100;


        me.calc_bonuses = _hyperborean_calc_bonuses;
        me.get_powers = _hyperborean_get_powers;
        me.get_flags = _hyperborean_get_flags;
        me.gain_level = _hyperborean_gain_level;
        init = TRUE;
    }

    return &me;
}