#ifndef MMX5_COOP_FORMS_H
#define MMX5_COOP_FORMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Co-op choices are owned by the committed co-op package. Offline standalone
 * packages remain independent of activation/configuration order. */
void mmx5_coop_forms_configure(int ultimate_x, int black_zero);
unsigned mmx5_coop_forms_enabled(unsigned character);
unsigned mmx5_coop_forms_roster(unsigned character, unsigned native_roster);

#ifdef __cplusplus
}
#endif

#endif
