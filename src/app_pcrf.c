#include "app_pcrf.h"

#include <freeDiameter/libfdproto.h>

/* static objects */
struct SAppPCRFConf *g_psoConf = NULL;
static struct SAppPCRFConf soConf;

/* initialyze instance */
static int app_pcrf_conf_init ()
{
	g_psoConf = &soConf;

	memset (g_psoConf, 0, sizeof (struct SAppPCRFConf));

	return 0;
}

/* entry point */
static int pcrf_entry (char * conffile)
{
	TRACE_ENTRY ("%p", conffile);

	/* Initialize configuration */
	CHECK_FCT (app_pcrf_conf_init ());

	/* Parse configuration file */
	if (conffile != NULL) {
		CHECK_FCT (app_pcrf_conf_handle (conffile));
	}

	/* Install objects definitions for this app_pcrf application */
	CHECK_FCT (app_pcrf_dict_init ());

	/* ������������� ���� ����������� � �� */
	CHECK_FCT (pcrf_db_pool_init ());

	/* Install the handlers for incoming messages */
	CHECK_FCT (app_pcrf_serv_init ());

	/* ������������� ������� (client) */
	CHECK_FCT (pcrf_cli_init ());

	/* Advertise the support for the Gx application in the peer */
	CHECK_FCT (fd_disp_app_support (g_psoDictApp, g_psoDictVend, 1, 0));

	return 0;
}

/* Unload */
void fd_ext_fini(void)
{
	app_pcrf_serv_fini ();
	pcrf_cli_fini ();
	pcrf_db_pool_fin ();
}

EXTENSION_ENTRY ("app_pcrf", pcrf_entry);
