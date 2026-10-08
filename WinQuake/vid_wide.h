typedef struct aspect_ratio_info
{
    char *name;
    float ratio;
}
aspect_ratio_info_t;

extern int aspect_ratio;

extern aspect_ratio_info_t aspect_ratio_list[];
extern const int aspect_ratio_count;

extern cvar_t wide_aspect_ratio;

extern void Wide_Init(void);

extern void Wide_LoadConfigValue(void);

extern int Wide_GetAspectRatio(void);

extern void Wide_SetAspectRatio(int n);
