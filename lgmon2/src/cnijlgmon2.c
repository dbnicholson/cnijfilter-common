/*
 *  Canon Inkjet Printer Driver for Linux
 *  Copyright CANON INC. 2013
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * NOTE:
 *  - As a special exception, this program is permissible to link with the
 *    libraries released as the binary modules.
 *  - If you write modifications of your own for these programs, it is your
 *    choice whether to permit this exception to apply to your modifications.
 *    If you do not wish that, delete this exception.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include "cnijcomif.h"
#include "cnijlgmon2.h"
#include "cnijifusb.h"
#include "keytext.h"

//#define _DEBUG_MODE_

int interrupt_sign = 0;
int (*CLSS_GetPrintCommand)(char *, int, int *, char *, int);
int (*CLSS_GetResponse)(char *, int, int *, char *, int **);
int (*CLSS_GetStatus)(char *, int, int *, int * , char *);

 int main(int argc, char *argv[])
{
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: lgmon2 started.\n");
#endif

	/*--------------param setting-----------------*/
	CNIF_INFO if_info;
	FILE *fp = stdin;
	//ppd_file_t *ppd;
	//ppd_attr_t *attr;
	int err = 0;
	int sign = 1;
	int copies = 0;
	int installer = 0;
	int response_detail = 0;
	size_t cmd_size = 0;				/* new command size */
	size_t transfered_data = 0;
	size_t readed_data = 0;
	size_t response_size = 0;
	char *cmd_buffer = NULL;
	char *write_buffer = NULL;
	char *job_id = NULL;
	char *temp_job = NULL;
	char *dev_uri = NULL;
	char *deviceID = NULL;
	char *library_path = NULL;
	//char *model_number = NULL;
	char model_number[] = "com";
	
	/*--------------localize init-----------------*/
	setlocale( LC_ALL, "" );
	bindtextdomain( PACKAGE, PACKAGE_LOCALE_DIR );
	bind_textdomain_codeset( PACKAGE, "UTF-8" );
	textdomain( PACKAGE );

	SetKeyTextDir( PACKAGE_DATA_DIR );
	if ( LoadKeyTextList() != 0 ){
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: lgmon2 LoadKeyTextList failed.\n");
#endif
		goto onErr;
	}

	/*--------------param init-----------------*/
	cmd_buffer = (char *)malloc(CN_READ_SIZE);
	if(cmd_buffer == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	write_buffer = (char *)malloc(CN_WRITE_SIZE);
	if(write_buffer == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	job_id = (char *)malloc(CN_IVEC_JOBID_LEN);
	if(job_id == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	temp_job = (char *)malloc(CN_IVEC_JOBID_LEN);
	if(temp_job == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	dev_uri = (char *)malloc(CN_DEVICE_URI_LEN);
	if(dev_uri == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	deviceID = (char *)malloc(CN_DEVICE_ID_LEN);
	if(deviceID == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	library_path = (char *)malloc(CN_LIB_PATH_LEN);
	if(library_path == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	/*model_number = (char *)malloc(CN_MODEL_ID_LEN);
	if(model_number == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}*/
	
	
	memset(cmd_buffer, 		'\0', CN_READ_SIZE);
	memset(write_buffer, 	'\0', CN_WRITE_SIZE);
	memset(job_id, 			'\0', CN_IVEC_JOBID_LEN);
	memset(temp_job, 		'\0', CN_IVEC_JOBID_LEN);
	memset(dev_uri, 		'\0', CN_DEVICE_URI_LEN);
	memset(deviceID, 		'\0', CN_DEVICE_ID_LEN);
	memset(library_path, 	'\0', CN_LIB_PATH_LEN);
	//memset(model_number, 	'\0', CN_MODEL_ID_LEN);
	
	CNIF_SetSignal(CN_SIGTERM);
	/*--------------switch option(argv[])-----------------*/
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: argc = %d\n", argc);
#endif
	switch(argc) {
		case 1:
			/* discover */
			err = CNIF_Discover(&if_info, installer);
			if (err < 0) {
				err = CN_LGMON_OTHER_ERROR;
			}
			return err;
			break;
		case 2:
			/* --installer_usb --search */
			if(!strncmp(CN_CMD_INSTALL_USB, argv[1], sizeof(CN_CMD_INSTALL_USB))){
				if_info.ifType = CNIF_TYPE_USB;
				installer = 1;
				err = CNIF_Discover(&if_info, installer);
				if (err < 0) {
					err = CN_LGMON_OTHER_ERROR;
				}
				return err;
			/* --installer_net --search */
			} else if(!strncmp(CN_CMD_INSTALL_NET, argv[1], sizeof(CN_CMD_INSTALL_NET))){
				if_info.ifType = CNIF_TYPE_NET;
				installer = 1;
				err = CNIF_Discover(&if_info, installer);
				if (err < 0) {
					err = CN_LGMON_OTHER_ERROR;
				}
				return err;
			}
#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: command error\n");
#endif
			err = CN_LGMON_CMD_ERROR;
			return err;;
			break;
		case 3:
			copies = atoi(argv[2]);
			break;
		default:
#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: command error\n");
#endif
			err = CN_LGMON_CMD_ERROR;
			return err;
			break;
	}
	
	fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PREPARING" ));
	
	/* read the file data before tansferdata */
	readed_data = fread(write_buffer, sizeof(char), CN_WRITE_SIZE, fp);
	
	/*--------------get ppd file------------------*/
/*	ppd = ppdOpenFile(getenv("PPD"));
	if (ppd != NULL){
		attr = ppdFindAttr(ppd, "cupsModelNumber", NULL);
		if (attr != NULL){
			snprintf(model_number, CN_MODEL_ID_LEN-1, "%s", attr->value);
		}
		ppdClose(ppd);
	} else {
		
#ifdef _DEBUG_MODE_
		fprintf(stderr, "ERROR: cannot open ppd file.\n");
#endif

		return CN_LGMON_PDD_FILE_ERROR;
	}
	if (model_number[0] == '\0'){
		
#ifdef _DEBUG_MODE_
		fprintf(stderr, "ERROR: cannot specify model number.\n");
#endif
		return CN_LGMON_PDD_FILE_ERROR;
	}
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: ppdfile = %s, model = %s\n", getenv("PPD"), model_number);
#endif
*/
	
	/*---------------dynamic link of library---------------------*/
	void *libclss = NULL;
	
	snprintf(library_path, CN_LIB_PATH_LEN - 1, CN_CNCL_LIB_PATH, model_number);
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: progpath = %s\n", library_path);
#endif

	libclss = dlopen(library_path, RTLD_LAZY);
	if(!libclss){
		fprintf(stderr, "ERROR: dynamic link error.(%s)", dlerror());
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}
	
	CLSS_GetPrintCommand = dlsym(libclss, "CLSS_GetPrintCommand");
	if(dlerror() != NULL){
		fprintf(stderr, "DEBUG: cannnot find function.(%s)", dlerror());
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}
	CLSS_GetResponse = dlsym(libclss, "CLSS_GetInfoResponse");
	if(dlerror() != NULL){
		fprintf(stderr, "DEBUG: cannnot find function.(%s)", dlerror());
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}
	CLSS_GetStatus = dlsym(libclss, "CLSS_GetStatus");
	if(dlerror() != NULL){
		fprintf(stderr, "DEBUG: cannnot find function.(%s)", dlerror());
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}
	
	/*--------------duplicate device uri-----------------*/
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: dynamic link ok!!\n");
#endif
	
	strncpy(dev_uri, argv[1], CN_DEVICE_URI_LEN);
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: dev_uri = %s\n", dev_uri);
#endif
	
	/*--------------get deviceID-----------------*/
	err = CNIF_GetSerialNum(&if_info, (uint8_t *)dev_uri, CN_DEVICE_URI_LEN, deviceID);
	if(err < 0) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: serial = %s\n", deviceID);
#endif
	
	/*--------------open device-----------------*/
	do {
		err = CNIF_Open(deviceID, &if_info);
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: CNIF_Open ret = %d\n", err);
#endif

		if(err < 0) {
			sleep(30);
		}
	} while (err < 0);
	
	/*---------------clear printer buffer----------------------*/
	err = CNIF_GetResponse(&if_info, temp_job, &response_detail);
	if(err < 0) {
		err = CN_LGMON_RESPONSE_ERROR;
		goto onErr;
	}
	
	while(1) {
		err = CNIF_GetResponse(&if_info, temp_job, &response_detail);
		if (err >= 0) {
			if(err == CN_IVEC_STATUS_DETAIL_BUSYING) {
				sleep(10);
				continue;
			}
			break;
		} else {
			err = CN_LGMON_RESPONSE_ERROR;
			goto onErr;
		}
	}
	
	/*--------------start session-----------------*/
	while (1){
		/* check intterrupt signal */
		if(interrupt_sign == 1){
			fprintf(stderr, "INFO: \n");
			goto onErr;
		}
	
		/* start session */
		err = CNIF_StartSession(&if_info);
		if (err < 0){
			err = CN_LGMON_OPEN_ERROR;
			goto onErr;
		}
		else if (err == CN_LGMON_OK){
			break;
		}
		else
		{
			// display status
			err = CNIF_GetResponse(&if_info, temp_job, &response_detail);
			
			// device is busy
			// fprintf(stderr, "INFO: %s\n", LookupText( "LBM_BUSY" ));
			sleep(4);
			continue;
		}
	}
	
	/*---------------StartJobCommand---------------------------*/
	strncpy(job_id, CN_START_JOBID, sizeof(CN_START_JOBID));
	err = CNIF_GetPrintCommand(cmd_buffer, &cmd_size, job_id, CN_IVEC_STARTJOB);
	if(err < 0) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	
	/* send startjob command */
	for( ; cmd_size != 0; cmd_size -= transfered_data) {
		err = CNIF_Write(&if_info, (uint8_t *)cmd_buffer, cmd_size, &transfered_data);
		if(err < 0) {

#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: send start job error(%d)\n", err);
#endif
			err = CN_LGMON_WRITEDATA_ERROR;
			goto onErr;
		}
		
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: send start job = %d\n", transfered_data);
#endif

	}
	/* check response */
	while(1) {
		/* check intterrupt signal */
		if(interrupt_sign == 1){
			fprintf(stderr, "INFO: \n");
			goto onErr;
		}
		sleep(1);
		err = CNIF_GetResponse(&if_info, job_id, &response_detail);
		
		if(err== CN_IVEC_STARTJOB){

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: -------startjob----%s----\n", job_id);
#endif
			break;
		} else if((err == CN_IVEC_VENDERSHIFT) || (err == CN_IVEC_ENDJOB) || (err == CN_IVEC_STATUS)){
			
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: returned is %d, not startjob_res\n", err);//DEBUG_MSG
#endif

		} else if(err < 0){
			err = CN_LGMON_RESPONSE_ERROR;
			goto onErr;
		}
	}
	
    /*---------------clear printer buffer----------------------*/
	memset(cmd_buffer, '\0', CN_READ_SIZE);
	while (response_size == CN_READ_SIZE) {
		err = CNIF_Read(&if_info, (uint8_t *)cmd_buffer, CN_READ_SIZE, &response_size);
		if(err < 0) {
			err = CN_LGMON_RESPONSE_ERROR;
			goto onErr;
		}
	}
	
    /*---------------VenderCommand-----------------------------*/
	err = CNIF_GetPrintCommand(cmd_buffer, &cmd_size, job_id, CN_IVEC_VENDERSHIFT);
	if(err < 0) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	/* send vendershift command */
	for ( ; cmd_size != 0; cmd_size -= transfered_data){
		err = CNIF_Write(&if_info, (uint8_t *)cmd_buffer, cmd_size, &transfered_data);
		if(err < 0) {
			
#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: send vender cmd error(%d)\n", err);
#endif
			err = CN_LGMON_WRITEDATA_ERROR;
			goto onErr;
		}

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: send vender cmd = %d\n", transfered_data);
#endif

	}
	
	while(1) {
		/* check intterrupt signal */
		if(interrupt_sign == 1){
			fprintf(stderr, "INFO: \n");
			goto onErr;
		}
		sleep(1);
		
		err = CNIF_GetResponse(&if_info, temp_job, &response_detail);
		
		if((err == CN_IVEC_VENDERSHIFT)){
			if(response_detail != CN_IVEC_NOT_START && strncmp(temp_job, job_id, CN_IVEC_JOBID_LEN)) {

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: -------vendercommand----other jobid %s----\n", temp_job);
#endif
			} else {
				
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: -------vendercommand----%s----\n", temp_job);
#endif
				break;
			}
		} else if((err == CN_IVEC_STARTJOB) || (err == CN_IVEC_ENDJOB) || (err == CN_IVEC_STATUS)){
			
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: returned is %d, not vendercommand_res\n", err);//DEBUG_MSG
#endif

		} else if(err < 0){
			err = CN_LGMON_RESPONSE_ERROR;
			goto onErr;
		}
	}
	
	fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
	
    /*---------------TransferData-----------------------------*/
	time_t start_time;
	time_t end_time;
	double past_sec = 0;
	time(&start_time);
	
	int times = 0;
	size_t max_read_size = 0;
	while(copies > 0) {
		copies--;
		 while(sign) {
			max_read_size = readed_data;
			
			if (readed_data < CN_WRITE_SIZE) {
				if (feof(fp)) {
					sign = 0;	//EOF
				}
				if (ferror(fp)) {

#ifdef _DEBUG_MODE_
					fprintf(stderr, "ERROR: fread error\n");//ERROR_MSG
#endif
					err = CN_LGMON_OTHER_ERROR;
					goto onErr;
				}
			}
			
			/* transfer data to printer */
			for( ; readed_data != 0; readed_data -= transfered_data) {
				
				/* check intterrupt signal */
				if(interrupt_sign == 1){
					//reset and close printer
					err = CNIF_Cancel(&if_info);
					err = CN_LGMON_OK;
					fprintf(stderr, "INFO: \n");
					goto onErr;
				}
				
				time(&end_time);
				past_sec = difftime(end_time, start_time);
				
				if(past_sec >= 4){
					time(&start_time);
					
					err = CNIF_GetResponse(&if_info, temp_job, &response_detail);
					if(err == CN_IVEC_STATUS){
						
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: -------status----%d----\n", err);
#endif

					} else if((err == CN_IVEC_STARTJOB) || (err == CN_IVEC_ENDJOB) || (err == CN_IVEC_VENDERSHIFT)){

#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: returned is %d, not status_res\n", err);//ERROR_MSG
#endif

					} else if(err < 0){
#ifdef _DEBUG_MODE_
						fprintf(stderr, "ERROR: Response error(%d)\n", err);
#endif
					}
				}
				
				err = CNIF_Write(&if_info, (uint8_t *)write_buffer + (max_read_size - readed_data), readed_data, &transfered_data);
				
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [%d]:readed data = %d, write data = %d\n", times, readed_data, transfered_data);
#endif

				if(err < 0) {
					
#ifdef _DEBUG_MODE_
					fprintf(stderr, "ERROR: Data send error(%d). transfered_data = %d\n", err, transfered_data);//ERROR_MSG
#endif
					//fprintf(stderr, "ERROR: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
					fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
					err = CN_LGMON_WRITEDATA_ERROR;
					goto onErr;
		    	}
			}
			times++;
			/* read the file data */
			readed_data = fread(write_buffer, sizeof(char), CN_WRITE_SIZE, fp);
			
		};
	}
	fprintf(stderr, "DEBUG: all data sended\n");
	
	/*---------------EndJob---------------------------------*/
	err = CNIF_GetPrintCommand(cmd_buffer, &cmd_size, job_id, CN_IVEC_ENDJOB);
	if(err < 0) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}
	
	/* send endjob command */
	for ( ; cmd_size != 0; cmd_size -= transfered_data){
		err = CNIF_Write(&if_info, (uint8_t *)cmd_buffer, cmd_size, &transfered_data);
		if(err < 0) {
			
#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: send end job error(%d)\n", err);
#endif
			err = CN_LGMON_WRITEDATA_ERROR;
			goto onErr;
		}

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: send end job = %d\n", transfered_data);
#endif

	}
	
    while(1) {
		/* check intterrupt signal */
		if(interrupt_sign == 1){
			fprintf(stderr, "INFO: \n");
			goto onErr;
		}
		sleep(4);
		
		err = CNIF_GetResponse(&if_info, temp_job, &response_detail);
		if(err == CN_IVEC_ENDJOB){
			if(response_detail != CN_IVEC_NOT_START && strncmp(temp_job, job_id, CN_IVEC_JOBID_LEN)) {

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: -------EndJob----other jobid %s----\n", temp_job);
#endif
			} else {
				
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: -------EndJob----%d----%s----\n", err, temp_job);
#endif
				break;
			}
		} else if((err == CN_IVEC_STARTJOB) || (err == CN_IVEC_VENDERSHIFT) || (err == CN_IVEC_STATUS)){

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: returned is %d, not endjob_res\n", err);//ERROR_MSG
#endif

		} else if(err < 0){
			err = CN_LGMON_RESPONSE_ERROR;
			goto onErr;
		}
	}
	
	/*-----------close dynamic link--------------*/
	if(libclss != NULL){
		dlclose(libclss);
	}
	
	fprintf(stderr, "INFO: \n");
	goto OK;
	
onErr:
	err = CNIF_Reset(&if_info);
	
OK:
	/*--------------close device-----------------*/
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: device close\n");
#endif
	err = CNIF_Close(&if_info);
	
	/*--------------free ptr-----------------*/
	free(cmd_buffer);
	free(write_buffer);
	free(job_id);
	free(temp_job);
	free(dev_uri);
	free(deviceID);
	free(library_path);
	//free(model_number);
	
	FreeKeyTextList();
	FreeKeyTextDir();
	
	return err;
}

/* set signal */
void CNIF_SetSignal(int sign)
{
	/*struct  sigaction action;
	//sigset
	sigset(sign, CNIF_SigCatch);
	
	//sigaction
	memset(&action, 0, sizeof(action));
	action.sa_handler = CNIF_SigCatch;
	sigaction(sign, &action, NULL);
	*/
	//signal
	if (SIG_ERR == signal(sign, CNIF_SigCatch)) {        
		fprintf(stderr, "DEBUG: failed to set signal handler\n"); 
		exit(1); 
	}
}

//signal catch
void CNIF_SigCatch(int sign)
{
	fprintf(stderr, "DEBUG: write_process catch the interrupt signal %d\n", sign);//DEBUG_MSG
	interrupt_sign = 1;
}

int CNIF_GetPrintCommand(char *cmdBuffer, size_t *cmdSize, char *job_id, int opration_id)
{
	int err = 0;
	memset(cmdBuffer, '\0', CN_READ_SIZE);
	
	err = CLSS_GetPrintCommand(cmdBuffer, CN_READ_SIZE, (int *)cmdSize, job_id, opration_id);
	if( err < 0) {
#ifdef _DEBUG_MODE_
		fprintf(stderr, "ERROR: get printcommand failed(%d)\n;", err);
#endif
	}
	
	return err;
}

int CNIF_GetResponse(CNIF_INFO *if_info, char *job_id, int *response_detail)
{
	/*---------------define parameter-------------------------*/
    int err = 0;
	int result = 0;
	int opration_id = 0;
	size_t read_size = 0;
	int sumsize = 0;
	int status = 0;
	int statusDetail = 0;
	char dum_id[] = "11111111";
	char *supportID = NULL;
	char *stat_msg = NULL;
	char *stat_tmp_msg = NULL;
	char *cmdBuffer = NULL;
	char *testBuffer = NULL;
	char *tmpBuffer = NULL;
	char *jobId = NULL;
	
	/*---------------init parameter-------------------------*/
	supportID = (char *)malloc(CN_IVEC_SUPPORT_CODE_LENGTH);
	if(supportID == NULL){
		err = -1;
		goto onErr;
	}
	
	stat_msg = (char *)malloc(CN_STAT_MSG_LEN);
	if(stat_msg == NULL){
		err = -1;
		goto onErr;
	}
	
	stat_tmp_msg = (char *)malloc(CN_STAT_MSG_LEN);
	if(stat_tmp_msg == NULL){
		err = -1;
		goto onErr;
	}

	cmdBuffer = (char *)malloc(CN_READ_SIZE);
	if(cmdBuffer == NULL){
		err = -1;
		goto onErr;
	}
	
	tmpBuffer = (char *)malloc(CN_READ_SIZE);
	if(tmpBuffer == NULL){
		err = -1;
		goto onErr;
	}
	
	jobId = (char *)malloc(CN_IVEC_JOBID_LEN);
	if(jobId == NULL){
		err = -1;
		goto onErr;
	}
	
	
	memset(jobId, 		'\0', CN_IVEC_JOBID_LEN);
	memset(cmdBuffer, 	'\0', CN_READ_SIZE);
	memset(tmpBuffer, 	'\0', CN_READ_SIZE);
	memset(supportID, 	'\0', CN_IVEC_SUPPORT_CODE_LENGTH);
	memset(stat_msg, 	'\0', CN_STAT_MSG_LEN);
	memset(stat_tmp_msg, 	'\0', CN_STAT_MSG_LEN);
	
	/*---------------get printer state info-------------------------*/
    int i = 1;
    while(1) {
		err = CNIF_Read(if_info, (uint8_t *)tmpBuffer, CN_READ_SIZE, &read_size);
		
#ifdef _DEBUG_MODE_
	//fprintf(stderr, "tmpBuffer = %s, readsize = %d, err = %d\n", tmpBuffer, (int)read_size, err);
#endif
		cmdBuffer = strncat(cmdBuffer, tmpBuffer, CN_READ_SIZE);
		sumsize += read_size;
				
#ifdef _DEBUG_MODE_
	//fprintf(stderr, "cmdBuffer = %s, buffersize = %d, sumsize = %d, err = %d\n", cmdBuffer, i * CN_READ_SIZE, sumsize, err);
#endif
		if (err == 0) {
			break;
		} else if (err == -8) {
			if (i * CN_READ_SIZE - sumsize < CN_READ_SIZE) {
				i = i + 1;
				
				testBuffer = (char *)realloc(cmdBuffer, i * CN_READ_SIZE);
				if (testBuffer == NULL) {
					err = -1;
					goto onErr;
				} else {
					memset(testBuffer + ((i - 1) * CN_READ_SIZE), '\0', CN_READ_SIZE);
					cmdBuffer = testBuffer;
					testBuffer = NULL;
				}
			}
			continue;
		} else {
			if (read_size == 0) {
				fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
			}
			goto onErr;
		}
	}
	
	err = CLSS_GetResponse(cmdBuffer, sumsize, &opration_id, jobId, &response_detail);
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "CLSS_GetResponse err = %d\n", err);
#endif

	if(err < 0 && err != -5) {
		goto onErr;
	}
	
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: opration id = %d, jobId = %s, response detail = %d\n", opration_id, jobId, *response_detail);
#endif
	
	switch(opration_id) {
		case CN_IVEC_START_RESPONSE:
		
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: sjob response = %s\n", jobId);
#endif
			result = CN_IVEC_STARTJOB;
			strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
			break;
		case CN_IVEC_VENDER_RESPONSE:
		
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: vjob response = %s\n", jobId);
#endif
			result = CN_IVEC_VENDERSHIFT;
			strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
			break;
		case CN_IVEC_END_RESPONSE:

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: ejob response = %s\n", jobId);
#endif
			result = CN_IVEC_ENDJOB;
			strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
			break;
		case CN_IVEC_STATUS_RESPONSE:
		
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: status response\n");
#endif
			//get printer status
			result = CLSS_GetStatus(cmdBuffer, sumsize, &status, &statusDetail, supportID);
			if(result < 0) {
				err = result;
				goto onErr;
			}
			
			if(*supportID != '\0') {
				//snprintf(stat_msg, CN_STAT_MSG_LEN - 1, "[support id: %s.] ", supportID);
				snprintf(stat_tmp_msg, CN_STAT_MSG_LEN - 1, "[%s] ", LookupText("LBM_ERROR_CODE"));
				snprintf(stat_msg, CN_STAT_MSG_LEN - 1, stat_tmp_msg, supportID);
			} else {
				/* supportID equel NULL */
			}
			/* check status */
			switch(status) {
				case CN_IVEC_STATUS_IDLE:
				case CN_IVEC_STATUS_PROCESSING:
					fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
					break;
				case CN_IVEC_STATUS_STOPPED:
				case CN_IVEC_STATUS_NOTREADY:
					/* check status detail */
					switch(statusDetail) {
						case CN_IVEC_STATUS_DETAIL_MEDIA_JAM:
							fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_JAM" ));
							break;
						case CN_IVEC_STATUS_DETAIL_DOOR_OPEN:
							fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_COVER_OPEN" ));
							break;
						case CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY:
							fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_PAPER_NOT_SET" ));
							break;
						case CN_IVEC_STATUS_DETAIL_BUSYING:
							//fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_BUSY" ));
							fprintf(stderr, "INFO: %s\n", LookupText( "LBM_BUSY" ));
							err = CN_IVEC_STATUS_DETAIL_BUSYING;
							goto onErr;
							break;
						default:
							if(*supportID != '\0'){
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_CHK_PRINTER" ));
							}
							break;
					}
					break;
				case CN_IVEC_STATUS_CANCELING:
					interrupt_sign = 1;
				default:
					break;
			}
			result = CN_IVEC_STATUS;
			strncpy(job_id, dum_id, CN_IVEC_JOBID_LEN);
			break;
		default:
		
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: ~~~default~~~\n");
#endif
			break;
	}
	
	err = result;
	
onErr:
	free(jobId);
	free(cmdBuffer);
	free(tmpBuffer);
	free(supportID);
	free(stat_msg);
	free(stat_tmp_msg);

	return err;
}

