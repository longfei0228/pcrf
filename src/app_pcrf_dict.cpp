#include "app_pcrf.h"

struct dict_object *g_psoDictVend = NULL;
struct dict_object *g_psoDictApp = NULL;
struct dict_object *g_psoDictCCR = NULL;
struct dict_object *g_psoDictCCA = NULL;
struct dict_object *g_psoDictRAR = NULL;
struct dict_object *g_psoDictRAA = NULL;
struct dict_object *g_psoDictASR = NULL;
struct dict_object *g_psoDictASA = NULL;
struct dict_object *g_psoDictAuthApplicationId = NULL;
struct dict_object *g_psoDictSessionID = NULL;
struct dict_object *g_psoDictDestHost = NULL;
struct dict_object *g_psoDictDestRealm = NULL;
struct dict_object *g_psoDictRARType = NULL;
struct dict_object *g_psoDictRC = NULL;
struct dict_object *g_psoDictOrignHost = NULL;
struct dict_object *g_psoDictOrignRealm = NULL;
struct dict_object *g_psoDictCCRequestType = NULL;
struct dict_object *g_psoDictCCRequestNumber = NULL;
struct dict_object *g_psoDictOriginStateId = NULL;

struct dict_object *g_psoDictSupportedFeatures = NULL;
struct dict_object *g_psoDictVendorId = NULL;
struct dict_object *g_psoDictFeatureListID = NULL;
struct dict_object *g_psoDictFeatureList = NULL;

struct dict_object *g_psoDictEventTrigger = NULL;

struct dict_object *g_psoDictChargingRuleInstall = NULL;
struct dict_object *g_psoDictChargingRuleRemove = NULL;
struct dict_object *g_psoDictBearerIdentifier = NULL;
struct dict_object *g_psoDictChargingRuleDefinition = NULL;
struct dict_object *g_psoDictChargingRuleBaseName = NULL;
struct dict_object *g_psoDictChargingRuleName = NULL;
struct dict_object *g_psoDictRatingGroup = NULL;
struct dict_object *g_psoDictServiceIdentifier = NULL;
struct dict_object *g_psoDictFlowDescription = NULL;
struct dict_object *g_psoDictFlowInformation = NULL;
struct dict_object *g_psoDictQoSInformation = NULL;
struct dict_object *g_psoDictQoSClassIdentifier = NULL;
struct dict_object *g_psoDictMaxRequestedBandwidthUL = NULL;
struct dict_object *g_psoDictMaxRequestedBandwidthDL = NULL;
struct dict_object *g_psoDictGuaranteedBitrateUL = NULL;
struct dict_object *g_psoDictGuaranteedBitrateDL = NULL;
struct dict_object *g_psoDictAllocationRetentionPriority = NULL;
struct dict_object *g_psoDictPriorityLevel = NULL;
struct dict_object *g_psoDictPreemptionCapability = NULL;
struct dict_object *g_psoDictPreemptionVulnerability = NULL;
struct dict_object *g_psoDictOnline = NULL;
struct dict_object *g_psoDictOffline = NULL;
struct dict_object *g_psoDictMeteringMethod = NULL;
struct dict_object *g_psoDictPrecedence = NULL;
struct dict_object *g_psoDictMonitoringKey = NULL;
struct dict_object *g_psoDictRedirectServer = NULL;
struct dict_object *g_psoDictRedirectAddressType = NULL;
struct dict_object *g_psoDictRedirectServerAddress = NULL;

struct dict_object *g_psoDictUsageMonitoringInformation = NULL;
struct dict_object *g_psoDictGrantedServiceUnit = NULL;
struct dict_object *g_psoDictCCTotalOctets = NULL;
struct dict_object *g_psoDictCCInputOctets = NULL;
struct dict_object *g_psoDictCCOutputOctets = NULL;
struct dict_object *g_psoDictUsageMonitoringLevel = NULL;
struct dict_object *g_psoDictUsageMonitoringReport = NULL;

struct dict_object *g_psoDictQoSUpgrade = NULL;
struct dict_object *g_psoDictQoSCI = NULL;
struct dict_object *g_psoDictIPCANType = NULL;
struct dict_object *g_psoDictRATType = NULL;
struct dict_object *g_psoDictQoSNegot = NULL;
struct dict_object *g_psoDictBearerUsage = NULL;
struct dict_object *g_psoDictBearerOper = NULL;
struct dict_object *g_psoDictPCCRuleStatus = NULL;
struct dict_object *g_psoDictRuleFailureCode = NULL;
struct dict_object *g_psoDictXHWMonitoringKey = NULL;
struct dict_object *g_psoDictXHWUsageReport = NULL;
struct dict_object *g_psoDictXHWSessionUsage = NULL;
struct dict_object *g_psoDictXHWServiceUsage = NULL;

struct local_rules_definition {
    dict_avp_request avp_codes;
    enum rule_position position;
    int min;
    int max;
};

#define RULE_ORDER( _position ) ((((_position) == RULE_FIXED_HEAD) || ((_position) == RULE_FIXED_TAIL)) ? 1 : 0 )

#define PARSE_loc_rules( _rulearray, _parent) {									\
	int __ar;																	\
	for (__ar=0; __ar < sizeof(_rulearray) / sizeof((_rulearray)[0]); __ar++) {	\
	    struct dict_rule_data __data = { NULL,									\
					     (_rulearray)[__ar].position,							\
					     0,														\
					     (_rulearray)[__ar].min,								\
					     (_rulearray)[__ar].max};								\
	    __data.rule_order = RULE_ORDER(__data.rule_position);					\
	    CHECK_FCT(  fd_dict_search(												\
			    fd_g_config->cnf_dict,											\
			    DICT_AVP,														\
			    AVP_BY_CODE_AND_VENDOR,											\
			    &((_rulearray)[__ar].avp_codes),								\
			    &__data.rule_avp, 0 ) );										\
	    if ( !__data.rule_avp ) {												\
		TRACE_DEBUG(INFO, "AVP Not found: vendor id: '%d'; avp code: '%d'", (_rulearray)[__ar].avp_codes.avp_vendor, (_rulearray)[__ar].avp_codes.avp_code );	\
		return ENOENT;															\
	    }																		\
	    CHECK_FCT_DO( fd_dict_new( fd_g_config->cnf_dict, DICT_RULE, &__data, _parent, NULL),				\
			  {																								\
			      TRACE_DEBUG(INFO, "Error on rule with AVP: vendor id: '%d'; avp code: '%d'",				\
					  (_rulearray)[__ar].avp_codes.avp_vendor, (_rulearray)[__ar].avp_codes.avp_code );		\
			      return EINVAL;												\
			  } );																\
	}																			\
}

int app_pcrf_dict_init (void)
{
	TRACE_DEBUG (FULL, "Initializing the dictionary for app_pcrf");

	command_code_t cctCmdCode;
	vendor_id_t vitVendId;
	struct dict_avp_request_ex soAVPIdent;
	struct dict_object **ppsoDictObj;
	struct dict_object *psoDictType;

	/* load vendor defination */
	vitVendId = 10415;
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_VENDOR, VENDOR_BY_ID, &vitVendId, &g_psoDictVend, ENOENT));

	/* ����������� � ������� ������� Credit-Contorl message */
	cctCmdCode = 272;
	/* load CCR */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_COMMAND, CMD_BY_CODE_R, &cctCmdCode, &g_psoDictCCR, ENOENT));
	/* load CCA */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_COMMAND, CMD_BY_CODE_A, &cctCmdCode, &g_psoDictCCA, ENOENT));

	/* ����������� � ������� ������� Re-Auth message */
	cctCmdCode = 258;
	/* load RAR */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_COMMAND, CMD_BY_CODE_R, &cctCmdCode, &g_psoDictRAR, ENOENT));
	/* load RAA */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_COMMAND, CMD_BY_CODE_A, &cctCmdCode, &g_psoDictRAA, ENOENT));

	/* ����������� � ������� ������� Abort-Session-message */
	cctCmdCode = 274;
	/* load ASR */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_COMMAND, CMD_BY_CODE_R, &cctCmdCode, &g_psoDictASR, ENOENT));
	/* load ASA */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_COMMAND, CMD_BY_CODE_A, &cctCmdCode, &g_psoDictASA, ENOENT));

	/* Create the app_pcrf Application */
	{
		struct dict_application_data soAppData;
		soAppData.application_id = 16777238;
		soAppData.application_name = (char *) "Gx";
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_APPLICATION, &soAppData, g_psoDictVend, &g_psoDictApp));
	}

	/* Auth-Application-Id */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Auth-Application-Id", &g_psoDictAuthApplicationId, ENOENT));

	/* Session-Id */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Session-Id", &g_psoDictSessionID, ENOENT));

	/* Destination-Host */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Destination-Host", &g_psoDictDestHost, ENOENT));

	/* Destination-Realm */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Destination-Realm", &g_psoDictDestRealm, ENOENT));

	/* Re-Auth-Request-Type */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Re-Auth-Request-Type", &g_psoDictRARType, ENOENT));

	/* Result-Code */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Result-Code", &g_psoDictRC, ENOENT));

	/* Origin-Host */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Origin-Host", &g_psoDictOrignHost, ENOENT));

	/* Origin-Realm */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Origin-Realm", &g_psoDictOrignRealm, ENOENT));

	/* CC-Request-Type */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "CC-Request-Type", &g_psoDictCCRequestType, ENOENT));

	/* CC-Request-Number */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "CC-Request-Number", &g_psoDictCCRequestNumber, ENOENT));

	/* Origin-State-Id */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Origin-State-Id", &g_psoDictOriginStateId, ENOENT));

	/* Supported-Features */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 628, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictSupportedFeatures, ENOENT));
	}

	/* Vendor-Id */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Vendor-Id", &g_psoDictVendorId, ENOENT));

	/* Feature-List-ID */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 629, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictFeatureListID, ENOENT));
	}

	/* Feature-List */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 630, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictFeatureList, ENOENT));
	}

	/* Event-Trigger */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1006, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictEventTrigger, ENOENT));
	}

	/* Charging-Rule-Install */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1001, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictChargingRuleInstall, ENOENT));
	}

	/* Charging-Rule-Remove */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1002, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictChargingRuleRemove, ENOENT));
	}

	/* Bearer-Identifier */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1020, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictBearerIdentifier, ENOENT));
	}

	/* Charging-Rule-Definition */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1003, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictChargingRuleDefinition, ENOENT));
	}

	/* Charging-Rule-Base-Name */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1004, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictChargingRuleBaseName, ENOENT));
	}

	/* Charging-Rule-Name */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1005, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictChargingRuleName, ENOENT));
	}

	/* Rating-Group */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Rating-Group", &g_psoDictRatingGroup, ENOENT));

	/* Service-Identifier */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Service-Identifier", &g_psoDictServiceIdentifier, ENOENT));

	/* Flow-Description */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 507, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictFlowDescription, ENOENT));
	}

	/* Flow-Information */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1058, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictFlowInformation, ENOENT));
	}

	/* QoS-Information */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1016, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictQoSInformation, ENOENT));
	}

	/* QoS-Class-Identifier */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1028, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictQoSClassIdentifier, ENOENT));
	}

	/* Max-Requested-Bandwidth-UL */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 516, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictMaxRequestedBandwidthUL, ENOENT));
	}

	/* Max-Requested-Bandwidth-DL */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 515, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictMaxRequestedBandwidthDL, ENOENT));
	}

	/* Guaranteed-Bitrate-UL */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1026, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictGuaranteedBitrateUL, ENOENT));
	}

	/* Guaranteed-Bitrate-DL */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1025, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictGuaranteedBitrateDL, ENOENT));
	}

	/* Allocation-Retention-Priority */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1034, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictAllocationRetentionPriority, ENOENT));
	}

	/* Priority-Level */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1046, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictPriorityLevel, ENOENT));
	}

	/* Pre-emption-Capability */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1047, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictPreemptionCapability, ENOENT));
	}

	/* Pre-emption-Vulnerability */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1048, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictPreemptionVulnerability, ENOENT));
	}

	/* Online */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1009, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictOnline, ENOENT));
	}

	/* Offline */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1008, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictOffline, ENOENT));
	}

	/* Metering-Method */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1007, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictMeteringMethod, ENOENT));
	}

	/* Precedence */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1010, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictPrecedence, ENOENT));
	}

	/* Monitoring-Key */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1066, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictMonitoringKey, ENOENT));
	}

	/* Redirect-Server */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Redirect-Server", &g_psoDictRedirectServer, ENOENT));

	/* Redirect-Address-Type */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Redirect-Address-Type", &g_psoDictRedirectAddressType, ENOENT));

	/* Redirect-Server-Address */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Redirect-Server-Address", &g_psoDictRedirectServerAddress, ENOENT));

	/* Usage-Monitoring-Information */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1067, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictUsageMonitoringInformation, ENOENT));
	}

	/* Granted-Service-Unit */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "Granted-Service-Unit", &g_psoDictGrantedServiceUnit, ENOENT));

	/* CC-Total-Octets */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "CC-Total-Octets", &g_psoDictCCTotalOctets, ENOENT));

	/* CC-Input-Octets */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "CC-Input-Octets", &g_psoDictCCInputOctets, ENOENT));

	/* CC-Output-Octets */
	CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_NAME, "CC-Output-Octets", &g_psoDictCCOutputOctets, ENOENT));

	/* Usage-Monitoring-Level */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1068, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictUsageMonitoringLevel, ENOENT));
	}

	/* Usage-Monitoring-Report */
	{
		dict_avp_request_ex soCrit = { { 0, 10415, NULL }, { 1069, NULL }};
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soCrit, &g_psoDictUsageMonitoringReport, ENOENT));
	}

	/* ��������� ������� ������������� ���������� */
	/* Online */
	{
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, g_psoDictOnline, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "DISABLE_ONLINE", { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "ENABLE_ONLINE",  { (uint8_t *) 1 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
	}
	/* Offline */
	{
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, g_psoDictOffline, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "DISABLE_OFFLINE", { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "ENABLE_OFFLINE",  { (uint8_t *) 1 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
	}
	/* QoS-Upgrade */
	{
		ppsoDictObj = &g_psoDictQoSUpgrade;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_vendor.vendor_id = 10415;
		soAVPIdent.avp_data.avp_code = 1030;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "QoS_UPGRADE_NOT_SUPPORTED", { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "QoS_UPGRADE_SUPPORTED",     { (uint8_t *) 1 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
	}
	/* QoS-Class-Identifier */
	{
		ppsoDictObj = &g_psoDictQoSCI;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_vendor.vendor_id = 10415;
		soAVPIdent.avp_data.avp_code = 1028;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
	}
	/* IP-CAN-Type */
	{
		ppsoDictObj = &g_psoDictIPCANType;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1027;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "3GPP-GPRS", { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "DOCSIS",    { (uint8_t *) 1 }};
		struct dict_enumval_data        t_3 = { (char *) "xDSL",      { (uint8_t *) 2 }};
		struct dict_enumval_data        t_4 = { (char *) "WiMAX",     { (uint8_t *) 3 }};
		struct dict_enumval_data        t_5 = { (char *) "3GPP2",     { (uint8_t *) 4 }};
		struct dict_enumval_data        t_6 = { (char *) "3GPP-EPS",  { (uint8_t *) 5 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_3 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_4 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_5 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_6 , psoDictType, NULL));
	}
	/* RAT-Type */
	{
		ppsoDictObj = &g_psoDictRATType;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1032;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "WLAN",           { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "UTRAN",          { (uint8_t *) 1000 }};
		struct dict_enumval_data        t_3 = { (char *) "GERAN",          { (uint8_t *) 1001 }};
		struct dict_enumval_data        t_4 = { (char *) "GAN",            { (uint8_t *) 1002 }};
		struct dict_enumval_data        t_5 = { (char *) "HSPA_EVOLUTION", { (uint8_t *) 1003 }};
		struct dict_enumval_data        t_6 = { (char *) "EUTRAN",         { (uint8_t *) 1004 }};
		struct dict_enumval_data        t_7 = { (char *) "CDMA2000_1X",    { (uint8_t *) 2000 }};
		struct dict_enumval_data        t_8 = { (char *) "HRPD",           { (uint8_t *) 2001 }};
		struct dict_enumval_data        t_9 = { (char *) "UMB",            { (uint8_t *) 2002 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_3 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_4 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_5 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_6 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_7 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_8 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_9 , psoDictType, NULL));
	}
	/* QoS-Negotiation */
	{
		ppsoDictObj = &g_psoDictQoSNegot;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1029;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "NO_QoS_NEGOTIATION",        { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "QoS_NEGOTIATION_SUPPORTED", { (uint8_t *) 1 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
	}
	/* Bearer-Usage */
	{
		ppsoDictObj = &g_psoDictBearerUsage;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1000;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "GENERAL",        { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "IMS_SIGNALLING", { (uint8_t *) 1 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
	}
	/* Bearer-Operation */
	{
		ppsoDictObj = &g_psoDictBearerOper;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1021;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "TERMINATION",   { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "ESTABLISHMENT", { (uint8_t *) 1 }};
		struct dict_enumval_data        t_3 = { (char *) "MODIFICATION",  { (uint8_t *) 2 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_3 , psoDictType, NULL));
	}
	/* PCC-Rule-Status */
	{
		ppsoDictObj = &g_psoDictPCCRuleStatus;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1019;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "ACTIVE",               { (uint8_t *) 0 }};
		struct dict_enumval_data        t_2 = { (char *) "INACTIVE",             { (uint8_t *) 1 }};
		struct dict_enumval_data        t_3 = { (char *) "TEMPORARILY INACTIVE", { (uint8_t *) 2 }};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_3 , psoDictType, NULL));
	}
	/* Rule-Failure-Code */
	{
		ppsoDictObj = &g_psoDictRuleFailureCode;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_data.avp_code = 1031;
		soAVPIdent.avp_vendor.vendor_id = 10415;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_TYPE, TYPE_OF_AVP, *ppsoDictObj, &psoDictType, ENOENT));
		struct dict_enumval_data        t_1 = { (char *) "UNKNOWN_RULE_NAME",            { (uint8_t *) 1 }};
		struct dict_enumval_data        t_2 = { (char *) "RATING_GROUP_ERROR",           { (uint8_t *) 2 }};
		struct dict_enumval_data        t_3 = { (char *) "SERVICE_IDENTIFIER_ERROR",     { (uint8_t *) 3 }};
		struct dict_enumval_data        t_4 = { (char *) "GW/PCEF_MALFUNCTION",          { (uint8_t *) 4 }};
		struct dict_enumval_data        t_5 = { (char *) "RESOURCES_LIMITATION",         { (uint8_t *) 5 }};
		struct dict_enumval_data        t_6 = { (char *) "MAX_NR_BEARERS_REACHED",       { (uint8_t *) 6 }};
		struct dict_enumval_data        t_7 = { (char *) "UNKNOWN_BEARER_ID",            { (uint8_t *) 7 }};
		struct dict_enumval_data        t_8 = { (char *) "MISSING_BEARER_ID",            { (uint8_t *) 8 }};
		struct dict_enumval_data        t_9 = { (char *) "MISSING_FLOW_DESCRIPTION",     { (uint8_t *) 9 }};
/*		struct dict_enumval_data        t_10 = { (char *) "MISSING_FLOW_INFORMATION",    { (uint8_t *) 9 }}; ������������ ��������� �������� */
		struct dict_enumval_data        t_11 = { (char *) "RESOURCE_ALLOCATION_FAILURE", { (uint8_t *) 10 }};
		struct dict_enumval_data        t_12 = { (char *) "UNSUCCESSFUL_QOS_VALIDATION", { (uint8_t *) 11 }};
		struct dict_enumval_data        t_13 = { (char *) "INCORRECT_FLOW_INFORMATION",  { (uint8_t *) 12 }};
		struct dict_enumval_data        t_14 = { (char *) "NO_BEARER_BOUND",             { (uint8_t *) 15 }};
		struct dict_enumval_data        t_15 = { (char *) "DUPLICATE_RULE_NAME ",        { (uint8_t *) 1001 }};
		struct dict_enumval_data        t_16 = { (char *) "FILTER_RESTRICTIONS",         { (uint8_t *) 1002 }};
		struct dict_enumval_data        t_17 = { (char *) "TIME_CONTROL_ERROR",          { (uint8_t *) 1004 }};
		struct dict_enumval_data        t_18 = { (char *) "L7_CONTENT_ERROR",            { (uint8_t *) 1005 }};
/*		struct dict_enumval_data        t_19 = { (char *) "L7_CONTENT_ERROR",            { (uint8_t *) 1004 }}; ������������ ��������� �������� */
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_1 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_2 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_3 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_4 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_5 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_6 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_7 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_8 , psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_9 , psoDictType, NULL));
/*		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_10, psoDictType, NULL)); */
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_11, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_12, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_13, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_14, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_15, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_16, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_17, psoDictType, NULL));
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_18, psoDictType, NULL));
/*		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_ENUMVAL, &t_19, psoDictType, NULL)); */
	}

	/* Huawai */
	{
        struct dict_vendor_data vendor_data = { 2011, (char *) "Huawai" };
        CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_VENDOR, &vendor_data, NULL, NULL));
	}

	/* X-HW-Monitoring-Key */
	{
		/* 
			Unsigned32. 
		*/
		struct dict_avp_data data = { 
		2008,									/* Code */
		2011,									/* Vendor */
		(char *) "X-HW-Monitoring-Key",			/* Name */
		AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY,	/* Fixed flags */
		AVP_FLAG_VENDOR,						/* Fixed flag values */
		AVP_TYPE_UNSIGNED32						/* base type of data */
		};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_AVP, &data , NULL, NULL));

		ppsoDictObj = &g_psoDictXHWMonitoringKey;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_vendor.vendor_id = 2011;
		soAVPIdent.avp_data.avp_code = 2008;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
	}

	/* X-HW-Session-Usage */
	{
		/*
			Grouped
		*/
		struct dict_object * avp;
		struct dict_avp_data data = {
		2006,									/* Code */
		2011,									/* Vendor */
		(char *) "X-HW-Session-Usage",			/* Name */
		AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY,	/* Fixed flags */
		AVP_FLAG_MANDATORY,						/* Fixed flag values */
		AVP_TYPE_GROUPED						/* base type of data */
		};
		struct local_rules_definition rules[] = {
			{  {0,412,NULL},	RULE_OPTIONAL,	-1, 1 },	/* "CC-Input-Octets" */
			{  {0,414,NULL},	RULE_OPTIONAL,	-1, 1 }		/* "CC-Output-Octets" */
		};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_AVP, &data , NULL, &avp));
		PARSE_loc_rules (rules, avp);

		ppsoDictObj = &g_psoDictXHWSessionUsage;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_vendor.vendor_id = 2011;
		soAVPIdent.avp_data.avp_code = 2006;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
	}

	/* X-HW-Service-Usage */
	{
		/*
			Grouped
		*/
		struct dict_object * avp;
		struct dict_avp_data data = {
		2007,									/* Code */
		2011,									/* Vendor */
		(char *) "X-HW-Service-Usage",			/* Name */
		AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY,	/* Fixed flags */
		AVP_FLAG_MANDATORY,						/* Fixed flag values */
		AVP_TYPE_GROUPED						/* base type of data */
		};
		struct local_rules_definition rules[] = {
			{  {0,432,NULL},	RULE_OPTIONAL,	-1, 1 },	/* "Rating-Group" */
			{  {0,439,NULL},	RULE_OPTIONAL,	-1, 1 },	/* "Service-Identifier" */
			{  {0,412,NULL},	RULE_OPTIONAL,	-1, 1 },	/* "CC-Input-Octets" */
			{  {0,414,NULL},	RULE_OPTIONAL,	-1, 1 }		/* "CC-Output-Octets" */
		};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_AVP, &data , NULL, &avp));
		PARSE_loc_rules (rules, avp);

		ppsoDictObj = &g_psoDictXHWServiceUsage;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_vendor.vendor_id = 2011;
		soAVPIdent.avp_data.avp_code = 2007;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
	}

	/* X-HW-Usage-Report */
	{
		/*
			Grouped
		*/
		struct dict_object * avp;
		struct dict_avp_data data = {
		2005,									/* Code */
		2011,									/* Vendor */
		(char *) "X-HW-Usage-Report",			/* Name */
		AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY,	/* Fixed flags */
		AVP_FLAG_MANDATORY,						/* Fixed flag values */
		AVP_TYPE_GROUPED						/* base type of data */
		};
		struct local_rules_definition rules[] = {
			{  {2011,2006,NULL},	RULE_OPTIONAL,	-1, 1 },	/* "X-HW-Session-Usage" */
			{  {2011,2007,NULL},	RULE_OPTIONAL,	-1, 1 }		/* "X-HW-Service-Usage" */
		};
		CHECK_FCT (fd_dict_new (fd_g_config->cnf_dict, DICT_AVP, &data , NULL, &avp));
		PARSE_loc_rules (rules, avp);

		ppsoDictObj = &g_psoDictXHWUsageReport;
		memset (&soAVPIdent, 0, sizeof (soAVPIdent));
		soAVPIdent.avp_vendor.vendor_id = 2011;
		soAVPIdent.avp_data.avp_code = 2005;
		CHECK_FCT (fd_dict_search (fd_g_config->cnf_dict, DICT_AVP, AVP_BY_STRUCT, &soAVPIdent, ppsoDictObj, ENOENT));
	}

	return 0;
}
