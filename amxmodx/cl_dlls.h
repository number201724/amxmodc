#pragma once

#include "interface.h"

typedef struct cl_dllexport_s
{
	union
	{
		int (*pfnInitialize)(cl_enginefunc_t *, int);
		struct cl_dllexport_s* func_tables;
	};
	int (*pfnHUD_Init)(void);
	union
	{
		int (*pfnHUD_VidInit)(void);
		struct cl_dllexport_s *vac_table;
	};
	int (*pfnHUD_Redraw)(float, int);
	int (*pfnHUD_UpdateClientData)(client_data_t *, float);
	int (*pfnHUD_Reset)(void);
	void (*pfnHUD_PlayerMove)(struct playermove_s *, int);
	void (*pfnHUD_PlayerMoveInit)(struct playermove_s *);
	char (*pfnHUD_PlayerMoveTexture)(char *);
	void (*pfnIN_ActivateMouse)(void);
	void (*pfnIN_DeactivateMouse)(void);
	void (*pfnIN_MouseEvent)(int);
	void (*pfnIN_ClearStates)(void);
	void (*pfnIN_Accumulate)(void);
	void (*pfnCL_CreateMove)(float, struct usercmd_s *, int);
	int (*pfnCL_IsThirdPerson)(void);
	void (*pfnCL_CameraOffset)(float *);
	struct kbutton_s *(*pfnKB_Find)(const char *);
	void (*pfnCAM_Think)(void);
	void (*pfnV_CalcRefdef)(struct ref_params_s *);
	int (*pfnHUD_AddEntity)(int, struct cl_entity_s *, const char *);
	void (*pfnHUD_CreateEntities)(void);
	void (*pfnHUD_DrawNormalTriangles)(void);
	void (*pfnHUD_DrawTransparentTriangles)(void);
	void (*pfnHUD_StudioEvent)(const struct mstudioevent_s *, const struct cl_entity_s *);
	void (*pfnHUD_PostRunCmd)(struct local_state_s *, struct local_state_s *, struct usercmd_s *, int, double, unsigned int);
	void (*pfnHUD_Shutdown)(void);
	void (*pfnHUD_TxferLocalOverrides)(struct entity_state_s *, const struct clientdata_s *);
	void (*pfnHUD_ProcessPlayerState)(struct entity_state_s *, struct entity_state_s *);
	void (*pfnHUD_TxferPredictionData)(struct entity_state_s *, const struct entity_state_s *, struct clientdata_s *, const struct clientdata_s *, struct weapon_data_s *, const struct weapon_data_s *);
	void (*pfnDemo_ReadBuffer)(int, unsigned char *);
	int (*pfnHUD_ConnectionlessPacket)(const struct netadr_s *, const char *, char *, int *);
	int (*pfnHUD_GetHullBounds)(int, float *, float *);
	void (*pfnHUD_Frame)(double);
	int (*pfnHUD_Key_Event)(int, int, const char *);
	void (*pfnHUD_TempEntUpdate)(double, double, double, struct tempent_s **, struct tempent_s **, int (*)(struct cl_entity_s *), void (*)(struct tempent_s *, float));
	struct cl_entity_s *(*pfnHUD_GetUserEntity)(int);
	void (*pfnHUD_VoiceStatus)(int, qboolean);
	void (*pfnHUD_DirectorMessage)(int, void *);
	int (*pfnHUD_GetStudioModelInterface)(int, struct r_studio_interface_s **, struct engine_studio_api_s *);
	int (*pfnHUD_ChatInputPosition)(int *, int *);
	int (*pfnHUD_GetPlayerTeam)(int);
	void *(*pfnClientFactory)(void);
}cl_dllexport_t;

typedef IBaseInterface* (__cdecl * fnCreateInterface)( const char *pName, int *pReturnCode );
typedef int (__cdecl * fnF)(cl_dllexport_t *cl_dllexport);