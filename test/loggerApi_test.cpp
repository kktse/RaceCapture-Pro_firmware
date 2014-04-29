/*
 * loggerApi_test.cpp
 *
 *  Created on: Jun 10, 2013
 *      Author: brent
 */
#include "loggerApi_test.h"

#include "api.h"
#include "loggerApi.h"
#include "mock_serial.h"
#include "imu.h"
#include "loggerConfig.h"
#include "channelMeta.h"
#include "jsmn.h"
#include "mod_string.h"
#include "modp_atonum.h"
#include "memory_mock.h"
#include "printk.h"
#include <string>
#include <fstream>
#include <streambuf>
#include "predictive_timer_2.h"

#define CPPUNIT_ASSERT_CLOSE_ENOUGH(ACTUAL, EXPECTED) CPPUNIT_ASSERT((abs((ACTUAL - EXPECTED)) < 0.00001))

#define JSON_TOKENS 10000
#define FILE_PREFIX string("json_api_files/")

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( LoggerApiTest );

char * LoggerApiTest::processApiGeneric(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());
	char *txBuffer = mock_getTxBuffer();
	return txBuffer;
}


void LoggerApiTest::stringToJson(string buffer, Object &json){
	std::stringstream stream;
	for (size_t i = 0; i < buffer.size(); i++){
		stream.put(buffer[i]);
	}
	try{
			Reader::Read(json, stream);
	}
	catch (json::Exception &e){
			throw ("Could not parse json string");
	}
}

void LoggerApiTest::assertGenericResponse(char *buffer, const char * messageName, int responseCode){
	static jsmn_parser parser;
	static jsmntok_t json_tok[JSON_TOKENS];
	jsmn_init(&parser);
	int r = jsmn_parse(&parser, buffer, json_tok, JSON_TOKENS);
	if (r == JSMN_SUCCESS){
		if (json_tok->type == JSMN_OBJECT && json_tok->size == 2){
			jsmntok_t * tok_msgName = json_tok + 1;
			jsmn_trimData(tok_msgName);
			if (strcmp(tok_msgName->data, messageName) != 0){
				CPPUNIT_FAIL("assertGenericResponse: message name does not match");
			}
			jsmntok_t * tok_resultInfo = json_tok + 2;
			if (tok_resultInfo->type== JSMN_OBJECT && tok_resultInfo->size == 2){
				jsmntok_t *tok_rc = tok_resultInfo + 1;
				jsmntok_t *tok_rcVal = tok_resultInfo + 2;
				jsmn_trimData(tok_rc);
				jsmn_trimData(tok_rcVal);
				if (strcmp("rc", tok_rc->data) != 0){
					CPPUNIT_FAIL("assertGenericResponse: rc element name does not match");
				}
				CPPUNIT_ASSERT_EQUAL(responseCode, modp_atoi(tok_rcVal->data));
			}
			else{
				CPPUNIT_FAIL("assertGenericResponse: did not see expected result object");
			}
		}
		else{
			CPPUNIT_FAIL("assertGenericResponse: expected object at root level");
		}
	}
	else{
		CPPUNIT_FAIL("assertGenericResponse: failed to parse");
	}
}

int LoggerApiTest::findAndReplace(string & source, const string find, const string replace)
{
    int num=0;
    string::size_type fLen = find.size();
    string::size_type rLen = replace.size();
    for (string::size_type pos=0; (pos=source.find(find, pos))!=string::npos; pos+=rLen)
    {
        num++;
        source.replace(pos, fLen, replace);
    }
    return num;
}

string LoggerApiTest::readFile(string filename){
	ifstream t(filename.c_str());
	if (!t.is_open()){
		t.open(string(FILE_PREFIX + filename).c_str());

	}
	if (!t.is_open()){
		throw ("Can not find file " + filename);
	}
	string str;

	t.seekg(0, ios::end);
	int length = t.tellg();
	str.reserve(length);
	t.seekg(0, ios::beg);

	str.assign((istreambuf_iterator<char>(t)),
				istreambuf_iterator<char>());

	findAndReplace(str,"\r","");
	findAndReplace(str,string("\n"),string(""));
	str.append("\r\n");
	return str;
}

void LoggerApiTest::setUp()
{
	LoggerConfig *config = getWorkingLoggerConfig();
	initApi();
	initialize_logger_config();
	setupMockSerial();
	imu_init(config);
	resetPredictiveTimer();
}


void LoggerApiTest::tearDown()
{
}

void LoggerApiTest::testUnescapeTextField(){
	{
		char test[] =  "test1";
		unescapeTextField(test);
		CPPUNIT_ASSERT_EQUAL(string("test1"), string(test));
	}

	{
		char test[] =  "test\\n1";
		unescapeTextField(test);
		CPPUNIT_ASSERT_EQUAL(string("test\n1"), string(test));
	}
	{
		char test[] =  "test\\r1";
		unescapeTextField(test);
		CPPUNIT_ASSERT_EQUAL(string("test\r1"), string(test));
	}
	{
		char test[] =  "test\\\\1";
		unescapeTextField(test);
		CPPUNIT_ASSERT_EQUAL(string("test\\1"), string(test));
	}
	{
		char test[] =  "test\\\"1";
		unescapeTextField(test);
		CPPUNIT_ASSERT_EQUAL(string("test\"1"), string(test));
	}

}

void LoggerApiTest::testGetMultipleAnalogCfg(){

	LoggerConfig *c = getWorkingLoggerConfig();
	for (int i = 0; i < CONFIG_ADC_CHANNELS; i++){

		ADCConfig *analogCfg = &c->ADCConfigs[i];

		analogCfg->cfg.channeId = CHANNEL_Analog1 + i;
		analogCfg->cfg.sampleRate = encodeSampleRate(100);
		analogCfg->linearScaling = 3.21 + i;
		analogCfg->scalingMode = i;

		for (int x = 0; x < ANALOG_SCALING_BINS; x++){
			analogCfg->scalingMap.rawValues[x] = i * x;
		}

		for (int x = 0; x < ANALOG_SCALING_BINS; x++){
			analogCfg->scalingMap.scaledValues[x] = float(i * x) + 0.1F;
		}
	}

	char * response = processApiGeneric("getAnalogCfg3.json");

	Object json;
	stringToJson(response, json);

	for (int i = 0; i < CONFIG_ADC_CHANNELS; i++){
		std::ostringstream stringStream;
		stringStream << i;
		Object &analogJson = json["analogCfg"][stringStream.str().c_str()];

		CPPUNIT_ASSERT_EQUAL(1 + i, (int)(Number)analogJson["id"]);
		CPPUNIT_ASSERT_EQUAL(100, (int)(Number)analogJson["sr"]);
		CPPUNIT_ASSERT_EQUAL(3.21F + i, (float)(Number)analogJson["linScal"]);
		CPPUNIT_ASSERT_EQUAL(i, (int)(Number)analogJson["scalMod"]);

		Object scalMap = (Object)analogJson["map"];
		Array raw = (Array)scalMap["raw"];
		Array scal = (Array)scalMap["scal"];

		for (int x = 0; x < ANALOG_SCALING_BINS; x++){
			CPPUNIT_ASSERT_EQUAL(i * x, (int)(Number)raw[x]);
		}
		for (int x = 0; x < ANALOG_SCALING_BINS; x++){
			CPPUNIT_ASSERT_EQUAL((float)(i * x) + 0.1F, (float)(Number)scal[x]);
		}
	}
}

void LoggerApiTest::testGetAnalogConfigFile(string filename, int index){

	LoggerConfig *c = getWorkingLoggerConfig();
	ADCConfig *analogCfg = &c->ADCConfigs[index];

	analogCfg->cfg.channeId = 1;
	analogCfg->cfg.sampleRate = encodeSampleRate(50);
	analogCfg->linearScaling = 1.234F;
	analogCfg->scalingMode = 2;
	analogCfg->filterAlpha = 0.6F;


	int i = 0;
	for (int x = 0; x < ANALOG_SCALING_BINS; i+=10,x++){
		analogCfg->scalingMap.rawValues[x] = i;
	}
	float iv = 0;
	for (int x = 0; x < ANALOG_SCALING_BINS; iv+=1.1,x++){
		analogCfg->scalingMap.scaledValues[x] = iv;
	}
	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	std::ostringstream stringStream;
	stringStream << index;
	Object &analogJson = json["analogCfg"][stringStream.str()];

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)analogJson["id"]);
	CPPUNIT_ASSERT_EQUAL(50, (int)(Number)analogJson["sr"]);
	CPPUNIT_ASSERT_EQUAL(1.234F, (float)(Number)analogJson["linScal"]);
	CPPUNIT_ASSERT_EQUAL(0.6F, (float)(Number)analogJson["alpha"]);
	CPPUNIT_ASSERT_EQUAL(2, (int)(Number)analogJson["scalMod"]);

	Object scalMap = (Object)analogJson["map"];
	Array raw = (Array)scalMap["raw"];
	Array scal = (Array)scalMap["scal"];

	i = 0;
	for (int x = 0; x < ANALOG_SCALING_BINS; i+=10, x++){
		CPPUNIT_ASSERT_EQUAL(i, (int)(Number)raw[x]);
	}
	iv = 0;
	for (int x = 0; x < ANALOG_SCALING_BINS; iv+=1.1, x++){
		CPPUNIT_ASSERT_EQUAL(iv, (float)(Number)scal[x]);
	}
}

void LoggerApiTest::testGetAnalogCfg(){
	testGetAnalogConfigFile("getAnalogCfg1.json", 1 );
	testGetAnalogConfigFile("getAnalogCfg2.json", 0 );

}

void LoggerApiTest::testSetAnalogConfigFile(string filename){

	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();

	ADCConfig *adcCfg = &c->ADCConfigs[0];


	CPPUNIT_ASSERT_EQUAL(1, (int)adcCfg->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(adcCfg->cfg.sampleRate));

	CPPUNIT_ASSERT_EQUAL(2, (int)adcCfg->scalingMode);
	CPPUNIT_ASSERT_EQUAL(1.234F, adcCfg->linearScaling);
	CPPUNIT_ASSERT_EQUAL(0.6F, adcCfg->filterAlpha);

	CPPUNIT_ASSERT_EQUAL(1, (int)adcCfg->scalingMap.rawValues[0]);
	CPPUNIT_ASSERT_EQUAL(2, (int)adcCfg->scalingMap.rawValues[1]);
	CPPUNIT_ASSERT_EQUAL(3, (int)adcCfg->scalingMap.rawValues[2]);
	CPPUNIT_ASSERT_EQUAL(4, (int)adcCfg->scalingMap.rawValues[3]);
	CPPUNIT_ASSERT_EQUAL(5, (int)adcCfg->scalingMap.rawValues[4]);

	CPPUNIT_ASSERT_EQUAL(1.1F, adcCfg->scalingMap.scaledValues[0]);
	CPPUNIT_ASSERT_EQUAL(1.2F, adcCfg->scalingMap.scaledValues[1]);
	CPPUNIT_ASSERT_EQUAL(1.3F, adcCfg->scalingMap.scaledValues[2]);
	CPPUNIT_ASSERT_EQUAL(1.4F, adcCfg->scalingMap.scaledValues[3]);
	CPPUNIT_ASSERT_EQUAL(1.5F, adcCfg->scalingMap.scaledValues[4]);

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setAnalogCfg", API_SUCCESS);

}

void LoggerApiTest::testSetAnalogCfg()
{
	testSetAnalogConfigFile("setAnalogCfg1.json");
	testSetAnalogConfigFile("setAnalogCfg2.json");
	testSetAnalogConfigFile("setAnalogCfg3.json");
}

void LoggerApiTest::testGetImuConfigFile(string filename, int index){
	LoggerConfig *c = getWorkingLoggerConfig();
	ImuConfig *imuCfg = &c->ImuConfigs[index];

	imuCfg->cfg.channeId = 1;
	imuCfg->cfg.sampleRate = encodeSampleRate(100);
	imuCfg->mode = 1;
	imuCfg->physicalChannel = 3;
	imuCfg->zeroValue = 1234;
	imuCfg->filterAlpha = 0.7F;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	std::ostringstream stringStream;
	stringStream << index;
	Object &imuJson = json["imuCfg"][stringStream.str()];

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)imuJson["id"]);
	CPPUNIT_ASSERT_EQUAL(100, (int)(Number)imuJson["sr"]);

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)imuJson["mode"]);
	CPPUNIT_ASSERT_EQUAL(3, (int)(Number)imuJson["chan"]);
	CPPUNIT_ASSERT_EQUAL(1234, (int)(Number)imuJson["zeroVal"]);
	CPPUNIT_ASSERT_EQUAL(0.7F, (float)(Number)imuJson["alpha"]);
}

void LoggerApiTest::testGetImuCfg(){
	testGetImuConfigFile("getImuCfg1.json", API_SUCCESS);
}

void LoggerApiTest::testSetImuConfigFile(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();

	ImuConfig *imuCfg = &c->ImuConfigs[0];

	CPPUNIT_ASSERT_EQUAL(33, (int)imuCfg->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(imuCfg->cfg.sampleRate));
	CPPUNIT_ASSERT_EQUAL(1, (int)imuCfg->mode);
	CPPUNIT_ASSERT_EQUAL(2, (int)imuCfg->physicalChannel);
	CPPUNIT_ASSERT_EQUAL(1234, (int)imuCfg->zeroValue);
	CPPUNIT_ASSERT_EQUAL(0.7F, imuCfg->filterAlpha);

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setImuCfg", API_SUCCESS);
}

void LoggerApiTest::testSetImuCfg(){
	testSetImuConfigFile("setImuCfg1.json");
}

void LoggerApiTest::testSetCellConfigFile(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();

	CellularConfig *cellCfg = &c->ConnectivityConfigs.cellularConfig;

	CPPUNIT_ASSERT_EQUAL(string("foo.xyz"), string(cellCfg->apnHost));
	CPPUNIT_ASSERT_EQUAL(string("blarg"), string(cellCfg->apnUser));
	CPPUNIT_ASSERT_EQUAL(string("blorg"), string(cellCfg->apnPass));

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setCellCfg", API_SUCCESS);
}

void LoggerApiTest::testSetCellCfg()
{
	testSetCellConfigFile("setCellCfg1.json");
}

void LoggerApiTest::testGetCellCfg(){
	LoggerConfig *c = getWorkingLoggerConfig();
	CellularConfig *cellCfg = &c->ConnectivityConfigs.cellularConfig;
	strcpy(cellCfg->apnHost, "my.host");
	strcpy(cellCfg->apnUser, "user1");
	strcpy(cellCfg->apnPass, "pass1");

	char *response = processApiGeneric("getCellCfg1.json");
	Object json;
	stringToJson(response, json);

	Object &cellJson = json["cellCfg"];

	string host = (String)cellJson["apnHost"];
	string user = (String)cellJson["apnUser"];
	string pass = (String)cellJson["apnPass"];

	CPPUNIT_ASSERT_EQUAL(string("my.host"), host);
	CPPUNIT_ASSERT_EQUAL(string("user1"), user);
	CPPUNIT_ASSERT_EQUAL(string("pass1"), pass);

}

void LoggerApiTest::testSetBtConfigFile(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();

	BluetoothConfig *btCfg = &c->ConnectivityConfigs.bluetoothConfig;

	CPPUNIT_ASSERT_EQUAL(string("myRacecar"), string(btCfg->deviceName));
	CPPUNIT_ASSERT_EQUAL(string("3311"), string(btCfg->passcode));
}

void LoggerApiTest::testSetBtCfg()
{
	testSetBtConfigFile("setBtCfg1.json");
}

void LoggerApiTest::testGetBtCfg(){
	LoggerConfig *c = getWorkingLoggerConfig();
	BluetoothConfig *btCfg = &c->ConnectivityConfigs.bluetoothConfig;
	strcpy(btCfg->deviceName,"myRacecar");
	strcpy(btCfg->passcode,"3311");

	char *response = processApiGeneric("getBtCfg1.json");
	Object json;
	stringToJson(response, json);

	Object &btJson = json["btCfg"];

	string name = (String)btJson["name"];
	string pass = (String)btJson["pass"];

	CPPUNIT_ASSERT_EQUAL(string("myRacecar"), name);
	CPPUNIT_ASSERT_EQUAL(string("3311"), pass);
}


void LoggerApiTest::testSetConnectivityCfgFile(string filename){
	LoggerConfig *c = getWorkingLoggerConfig();
	ConnectivityConfig *connCfg = &c->ConnectivityConfigs;
	connCfg->sdLoggingMode = 0;
	connCfg->connectivityMode = 0;
	connCfg->backgroundStreaming = 0;

	processApiGeneric(filename);

	CPPUNIT_ASSERT_EQUAL(1, (int)connCfg->sdLoggingMode);
	CPPUNIT_ASSERT_EQUAL(1, (int)connCfg->connectivityMode);
	CPPUNIT_ASSERT_EQUAL(1, (int)connCfg->backgroundStreaming);

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setConnCfg", API_SUCCESS);
}

void LoggerApiTest::testSetConnectivityCfg(){
	testSetConnectivityCfgFile("setConnCfg1.json");
}

void LoggerApiTest::testGetConnectivityCfg(){
	LoggerConfig *c = getWorkingLoggerConfig();
	ConnectivityConfig *btCfg = &c->ConnectivityConfigs;
	btCfg->sdLoggingMode = 0;
	btCfg->connectivityMode = 2;

	char *response = processApiGeneric("getConnCfg1.json");
	Object json;
	stringToJson(response, json);

	Object &connJson = json["connCfg"];

	int sdMode = (Number)connJson["sdMode"];
	int connMode = (Number)connJson["connMode"];
	int bgStream = (Number)connJson["bgStream"];

	CPPUNIT_ASSERT_EQUAL(0, sdMode);
	CPPUNIT_ASSERT_EQUAL(2, connMode);
	CPPUNIT_ASSERT_EQUAL(1, bgStream);
}

void LoggerApiTest::testGetPwmConfigFile(string filename, int index){
	LoggerConfig *c = getWorkingLoggerConfig();
	PWMConfig *pwmCfg = &c->PWMConfigs[index];

	pwmCfg->cfg.channeId = 1;
	pwmCfg->cfg.sampleRate = encodeSampleRate(100);
	pwmCfg->outputMode = 3;
	pwmCfg->loggingMode = 1;
	pwmCfg->startupDutyCycle = 55;
	pwmCfg->startupPeriod = 321;
	pwmCfg->voltageScaling = 1.23;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	std::ostringstream stringStream;
	stringStream << index;
	Object &analogJson = json["pwmCfg"][stringStream.str()];

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)analogJson["id"]);
	CPPUNIT_ASSERT_EQUAL(100, (int)(Number)analogJson["sr"]);
	CPPUNIT_ASSERT_EQUAL(3, (int)(Number)analogJson["outMode"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)analogJson["logMode"]);
	CPPUNIT_ASSERT_EQUAL(55, (int)(Number)analogJson["stDutyCyc"]);
	CPPUNIT_ASSERT_EQUAL(321, (int)(Number)analogJson["stPeriod"]);
	CPPUNIT_ASSERT_EQUAL(1.23F, (float)(Number)analogJson["vScal"]);
}

void LoggerApiTest::testGetPwmCfg(){
	testGetPwmConfigFile("getPwmCfg1.json", 0);
}


void LoggerApiTest::testSetPwmConfigFile(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();
	PWMConfig *pwmCfg = &c->PWMConfigs[0];
	CPPUNIT_ASSERT_EQUAL(100, decodeSampleRate(pwmCfg->cfg.sampleRate));
	CPPUNIT_ASSERT_EQUAL(39, (int)pwmCfg->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL(1, (int)pwmCfg->outputMode);
	CPPUNIT_ASSERT_EQUAL(1, (int)pwmCfg->loggingMode);
	CPPUNIT_ASSERT_EQUAL(50, (int)pwmCfg->startupDutyCycle);
	CPPUNIT_ASSERT_EQUAL(110, (int)pwmCfg->startupPeriod);
	CPPUNIT_ASSERT_EQUAL(2.5F, (float)pwmCfg->voltageScaling);

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setPwmCfg", API_SUCCESS);

}

void LoggerApiTest::testSetPwmCfg(){
	testSetPwmConfigFile("setPwmCfg1.json");
}

void LoggerApiTest::testGetGpioConfigFile(string filename, int index){
	LoggerConfig *c = getWorkingLoggerConfig();
	GPIOConfig *gpioCfg = &c->GPIOConfigs[index];

	gpioCfg->cfg.channeId = 1;
	gpioCfg->cfg.sampleRate = encodeSampleRate(100);
	gpioCfg->mode = 1;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	std::ostringstream stringStream;
	stringStream << index;
	Object &analogJson = json["gpioCfg"][stringStream.str()];

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)analogJson["id"]);
	CPPUNIT_ASSERT_EQUAL(100, (int)(Number)analogJson["sr"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)analogJson["mode"]);
}

void LoggerApiTest::testGetGpioCfg(){
	testGetGpioConfigFile("getGpioCfg1.json", 0);
}

void LoggerApiTest::testSetGpioConfigFile(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();
	GPIOConfig *gpioCfg = &c->GPIOConfigs[0];

	CPPUNIT_ASSERT_EQUAL(55, (int)gpioCfg->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL(100, decodeSampleRate(gpioCfg->cfg.sampleRate));
	CPPUNIT_ASSERT_EQUAL(1, (int)gpioCfg->mode);

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setGpioCfg", API_SUCCESS);

}

void LoggerApiTest::testSetGpioCfg(){
	testSetGpioConfigFile("setGpioCfg1.json");
}

void LoggerApiTest::testGetTimerConfigFile(string filename, int index){
	LoggerConfig *c = getWorkingLoggerConfig();
	TimerConfig *timerCfg = &c->TimerConfigs[index];

	timerCfg->cfg.channeId = 1;
	timerCfg->cfg.sampleRate = encodeSampleRate(100);
	timerCfg->slowTimerEnabled = 1;
	timerCfg->mode = 2;
	timerCfg->filterAlpha = 0.5F;
	timerCfg->pulsePerRevolution = 3;
	timerCfg->timerDivider = 30000;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	std::ostringstream stringStream;
	stringStream << index;
	Object &timerJson = json["timerCfg"][stringStream.str()];

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)timerJson["id"]);
	CPPUNIT_ASSERT_EQUAL(100, (int)(Number)timerJson["sr"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)timerJson["sTimer"]);
	CPPUNIT_ASSERT_EQUAL(2, (int)(Number)timerJson["mode"]);
	CPPUNIT_ASSERT_EQUAL(0.5F, (float)(Number)timerJson["alpha"]);
	CPPUNIT_ASSERT_EQUAL(3, (int)(Number)timerJson["ppRev"]);
	CPPUNIT_ASSERT_EQUAL(30000, (int)(Number)timerJson["timDiv"]);
}

void LoggerApiTest::testGetTimerCfg(){
	testGetTimerConfigFile("getTimerCfg1.json", API_SUCCESS);
}

void LoggerApiTest::testSetTimerConfigFile(string filename){
	Serial *serial = getMockSerial();
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)json.c_str(), json.size());

	LoggerConfig *c = getWorkingLoggerConfig();
	TimerConfig *timerCfg = &c->TimerConfigs[0];

	CPPUNIT_ASSERT_EQUAL(10, decodeSampleRate(timerCfg->cfg.sampleRate));
	CPPUNIT_ASSERT_EQUAL(22, (int)timerCfg->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL(1, (int)timerCfg->slowTimerEnabled);
	CPPUNIT_ASSERT_EQUAL(1, (int)timerCfg->mode);
	CPPUNIT_ASSERT_EQUAL(0.5F, timerCfg->filterAlpha);
	CPPUNIT_ASSERT_EQUAL(4, (int)timerCfg->pulsePerRevolution);
	CPPUNIT_ASSERT_EQUAL(2, (int)timerCfg->timerDivider);

	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer, "setTimerCfg", API_SUCCESS);
}

void LoggerApiTest::testSetTimerCfg(){
	testSetTimerConfigFile("setTimerCfg1.json");
}

void LoggerApiTest::testGetMeta(){
	testSampleDataFile("getMeta.json", "getMeta_response.json");
}

void LoggerApiTest::testSampleData(){
	testSampleDataFile("sampleData1.json", "sampleData_response1.json");
	testSampleDataFile("sampleData2.json", "sampleData_response2.json");
}

void LoggerApiTest::testSampleDataFile(string requestFilename, string responseFilename){

	string requestJson = readFile(requestFilename);
	string expectedResponseJson = readFile(responseFilename);

	mock_resetTxBuffer();
	process_api(getMockSerial(),(char *)requestJson.c_str(), requestJson.size());

	LoggerConfig *c = getWorkingLoggerConfig();
	char *txBuffer = mock_getTxBuffer();
	string txString(txBuffer);
	CPPUNIT_ASSERT_EQUAL(expectedResponseJson, txString );
}

void LoggerApiTest::testLogStartStopFile(string filename){

	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(), (char *)json.c_str(), json.size());
	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer,"log", API_SUCCESS);
}

void LoggerApiTest::testLogStartStop(){
	testLogStartStopFile("log1.json");
	testLogStartStopFile("log2.json");
}

void LoggerApiTest::testCalibrateImuFile(string filename){

	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(), (char *)json.c_str(), json.size());
	char *txBuffer = mock_getTxBuffer();
	assertGenericResponse(txBuffer,"calImu",1);
}

void LoggerApiTest::testCalibrateImu(){
	testCalibrateImuFile("calibrateImu.json");
}

void LoggerApiTest::testFlashConfigFile(string filename){
	memory_mock_set_is_flashed(1);
	string json = readFile(filename);
	mock_resetTxBuffer();
	process_api(getMockSerial(), (char *)json.c_str(), json.size());
	char *txBuffer = mock_getTxBuffer();
	int isFlashed = memory_mock_get_is_flashed();
	CPPUNIT_ASSERT_EQUAL(isFlashed, 1);
	assertGenericResponse(txBuffer,"flashCfg",1);
}

void LoggerApiTest::testFlashConfig(){
	testFlashConfigFile("flashCfg.json");
}

void LoggerApiTest::testSetGpsConfigFile(string filename, unsigned char channelsEnabled, unsigned short sampleRate){
	processApiGeneric(filename);
	char *txBuffer = mock_getTxBuffer();

	LoggerConfig *c = getWorkingLoggerConfig();
	GPSConfig *gpsCfg = &c->GPSConfigs;

	assertGenericResponse(txBuffer, "setGpsCfg", API_SUCCESS);

	CPPUNIT_ASSERT_EQUAL((unsigned short)sampleRate, (unsigned short)decodeSampleRate(gpsCfg->sampleRate));

	CPPUNIT_ASSERT_EQUAL(channelsEnabled, gpsCfg->distanceEnabled);
	CPPUNIT_ASSERT_EQUAL(channelsEnabled, gpsCfg->positionEnabled);
	CPPUNIT_ASSERT_EQUAL(channelsEnabled, gpsCfg->satellitesEnabled);
	CPPUNIT_ASSERT_EQUAL(channelsEnabled, gpsCfg->speedEnabled);
	CPPUNIT_ASSERT_EQUAL(channelsEnabled, gpsCfg->timeEnabled);
}

void LoggerApiTest::testSetGpsCfg(){
	testSetGpsConfigFile("setGpsCfg1.json", 1, 100);
	testSetGpsConfigFile("setGpsCfg2.json", 0, 50);
}

void LoggerApiTest::testGetGpsConfigFile(string filename){
	LoggerConfig *c = getWorkingLoggerConfig();
	GPSConfig *gpsCfg = &c->GPSConfigs;

	gpsCfg->sampleRate = encodeSampleRate(100);

	gpsCfg->distanceEnabled = 1;
	gpsCfg->positionEnabled = 1;
	gpsCfg->satellitesEnabled = 1;
	gpsCfg->speedEnabled = 1;
	gpsCfg->timeEnabled = 1;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	Object &gpsCfgJson = json["gpsCfg"];

	CPPUNIT_ASSERT_EQUAL((int)100, (int)(Number)gpsCfgJson["sr"]);

	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)gpsCfgJson["dist"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)gpsCfgJson["pos"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)gpsCfgJson["sats"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)gpsCfgJson["speed"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)gpsCfgJson["time"]);
}

void LoggerApiTest::testGetGpsCfg(){
	testGetGpsConfigFile("getGpsCfg1.json");
}

void LoggerApiTest::testSetLapCfg(){
	testSetLapConfigFile("setLapCfg1.json");
}

void LoggerApiTest::testSetLapConfigFile(string filename){
	processApiGeneric(filename);
	char *txBuffer = mock_getTxBuffer();

	LoggerConfig *c = getWorkingLoggerConfig();
	LapConfig *cfg = &c->LapConfigs;

	assertGenericResponse(txBuffer, "setLapCfg", API_SUCCESS);

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_LapCount, (int) cfg->lapCountCfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(cfg->lapCountCfg.sampleRate));

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_LapTime, (int)cfg->lapTimeCfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(cfg->lapTimeCfg.sampleRate));

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_PredTime, (int)cfg->predTimeCfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(cfg->predTimeCfg.sampleRate));

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_Sector, (int)cfg->sectorCfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(cfg->predTimeCfg.sampleRate));

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_SectorTime, (int)cfg->sectorTimeCfg.channeId);
	CPPUNIT_ASSERT_EQUAL(50, decodeSampleRate(cfg->predTimeCfg.sampleRate));

}

void LoggerApiTest::testGetLapConfigFile(string filename){

	LoggerConfig *c = getWorkingLoggerConfig();
	LapConfig *cfg = &c->LapConfigs;
	cfg->lapCountCfg.sampleRate = encodeSampleRate(50);
	cfg->lapTimeCfg.sampleRate = encodeSampleRate(50);
	cfg->predTimeCfg.sampleRate = encodeSampleRate(50);
	cfg->sectorCfg.sampleRate = encodeSampleRate(50);
	cfg->sectorTimeCfg.sampleRate = encodeSampleRate(50);

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);
	{
		Object &channel = json["lapCfg"]["lapCount"];
		CPPUNIT_ASSERT_EQUAL((int)CHANNEL_LapCount, (int)(Number)channel["id"]);
		CPPUNIT_ASSERT_EQUAL(50, (int)(Number)channel["sr"]);
	}
	{
		Object &channel = json["lapCfg"]["lapTime"];
		CPPUNIT_ASSERT_EQUAL((int)CHANNEL_LapTime, (int)(Number)channel["id"]);
		CPPUNIT_ASSERT_EQUAL(50, (int)(Number)channel["sr"]);
	}
	{
		Object &channel = json["lapCfg"]["predTime"];
		CPPUNIT_ASSERT_EQUAL((int)CHANNEL_PredTime, (int)(Number)channel["id"]);
		CPPUNIT_ASSERT_EQUAL(50, (int)(Number)channel["sr"]);
	}
	{
		Object &channel = json["lapCfg"]["sector"];
		CPPUNIT_ASSERT_EQUAL((int)CHANNEL_Sector, (int)(Number)channel["id"]);
		CPPUNIT_ASSERT_EQUAL(50, (int)(Number)channel["sr"]);
	}
	{
		Object &channel = json["lapCfg"]["sectorTime"];
		CPPUNIT_ASSERT_EQUAL((int)CHANNEL_SectorTime, (int)(Number)channel["id"]);
		CPPUNIT_ASSERT_EQUAL(50, (int)(Number)channel["sr"]);
	}

}

void LoggerApiTest::testGetLapCfg(){
	testGetLapConfigFile("getLapCfg1.json");
}

void LoggerApiTest::testSetTrackConfigFile(string filename, int sectors){
	processApiGeneric(filename);
	char *txBuffer = mock_getTxBuffer();

	LoggerConfig *c = getWorkingLoggerConfig();
	TrackConfig *cfg = &c->TrackConfigs;

	assertGenericResponse(txBuffer, "setTrackCfg", API_SUCCESS);

	CPPUNIT_ASSERT_EQUAL(1, (int)cfg->track.track_type);
	CPPUNIT_ASSERT_CLOSE_ENOUGH(0.0001F, cfg->radius);
	CPPUNIT_ASSERT_EQUAL(0, (int)cfg->auto_detect);
	CPPUNIT_ASSERT_CLOSE_ENOUGH(1.1F, cfg->track.startFinish.latitude);
	CPPUNIT_ASSERT_CLOSE_ENOUGH(2.1F, cfg->track.startFinish.longitude);

	float startingValue = 1.1;

	for (int i = 0; i < sectors; i++){
		CPPUNIT_ASSERT_CLOSE_ENOUGH(startingValue, cfg->track.sectors[i].latitude);
		startingValue++;
		CPPUNIT_ASSERT_CLOSE_ENOUGH(startingValue, cfg->track.sectors[i].longitude);
		startingValue++;
	}
}

void LoggerApiTest::testSetTrackCfg(){
	testSetTrackConfigFile("setTrackCfg1.json", 10);
}

void LoggerApiTest::testGetTrackConfigFile(string filename){
	LoggerConfig *c = getWorkingLoggerConfig();
	TrackConfig *cfg = &c->TrackConfigs;

	float startingValue = 1.1;
	for (size_t i = 0; i < SECTOR_COUNT; i++){
		GeoPoint *point = (cfg->track.sectors + i);
		point->latitude = startingValue;
		startingValue++;
		point->longitude = startingValue;
		startingValue++;
	}
	cfg->radius = 0.009;
	cfg->auto_detect = 0;
	cfg->track.track_type = 1;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	CPPUNIT_ASSERT_EQUAL(0.009F, (float)(Number)json["trackCfg"]["rad"]);
	CPPUNIT_ASSERT_EQUAL(0, (int)(Number)json["trackCfg"]["autoDetect"]);
	CPPUNIT_ASSERT_EQUAL(1, (int)(Number)json["trackCfg"]["track"]["type"]);

	Array &sectors = json["trackCfg"]["track"]["sec"];

	startingValue = 1.1;
	for (size_t i = 0; i < SECTOR_COUNT; i++){
		CPPUNIT_ASSERT_CLOSE_ENOUGH(startingValue, (float)(Number) sectors[i][0]);
		startingValue++;
		CPPUNIT_ASSERT_CLOSE_ENOUGH(startingValue, (float)(Number) sectors[i][1]);
		startingValue++;
	}
}

void LoggerApiTest::testGetTrackCfg(){
	testGetTrackConfigFile("getTrackCfg1.json");
}

void LoggerApiTest::testSetLogLevelFile(string filename, int expectedResponse){
	processApiGeneric(filename);

	//set_log_level(DEBUG);
	//char *txBuffer = mock_getTxBuffer();
	CPPUNIT_ASSERT_EQUAL( 7, (int)get_log_level());
	//assertGenericResponse(txBuffer, "setLogfileLevel", expectedResponse);
}

void LoggerApiTest::testSetLogLevel(){
	testSetLogLevelFile("setLogLevel1.json", API_SUCCESS);
}

void LoggerApiTest::testSetObd2Cfg(){
	testSetObd2ConfigFile("setObd2Cfg1.json");
}

void LoggerApiTest::testGetObd2Cfg(){
	testGetObd2ConfigFile("getObd2Cfg1.json");
}


void LoggerApiTest::testGetObd2ConfigFile(string filename){
	LoggerConfig *c = getWorkingLoggerConfig();
	OBD2Config *obd2Config = &c->OBD2Configs;

	obd2Config->enabledPids = 2;
	obd2Config->pids[0].cfg.channeId = CHANNEL_AFR;
	obd2Config->pids[0].cfg.sampleRate = SAMPLE_1Hz;
	obd2Config->pids[0].pid = 0x05;

	obd2Config->pids[1].cfg.channeId = CHANNEL_Boost;
	obd2Config->pids[1].cfg.sampleRate = SAMPLE_50Hz;
	obd2Config->pids[1].pid = 0x06;

	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	Object pid1 = (Object)json["obd2Cfg"]["pids"][0];
	Object pid2 = (Object)json["obd2Cfg"]["pids"][1];

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_AFR, (int)(Number)pid1["id"]);
	CPPUNIT_ASSERT_EQUAL(SAMPLE_1Hz, (int)(Number)pid1["sr"]);
	CPPUNIT_ASSERT_EQUAL(0x05, (int)(Number)pid1["pid"]);

	CPPUNIT_ASSERT_EQUAL((int)CHANNEL_Boost, (int)(Number)pid2["id"]);
	CPPUNIT_ASSERT_EQUAL(SAMPLE_50Hz, (int)(Number)pid2["sr"]);
	CPPUNIT_ASSERT_EQUAL(0x06, (int)(Number)pid2["pid"]);
}

void LoggerApiTest::testSetObd2ConfigFile(string filename){
	processApiGeneric(filename);
	char *txBuffer = mock_getTxBuffer();

	LoggerConfig *c = getWorkingLoggerConfig();
	OBD2Config *obd2Config = &c->OBD2Configs;

	CPPUNIT_ASSERT_EQUAL(2, (int)obd2Config->enabledPids);

	PidConfig *pidCfg1 = &obd2Config->pids[0];
	PidConfig *pidCfg2 = &obd2Config->pids[1];

	CPPUNIT_ASSERT_EQUAL(19, (int)pidCfg1->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL((int)SAMPLE_1Hz, (int)pidCfg1->cfg.sampleRate);
	CPPUNIT_ASSERT_EQUAL(5, (int)pidCfg1->pid);

	CPPUNIT_ASSERT_EQUAL(26, (int)pidCfg2->cfg.channeId);
	CPPUNIT_ASSERT_EQUAL((int)SAMPLE_50Hz, (int)pidCfg2->cfg.sampleRate);
	CPPUNIT_ASSERT_EQUAL(6, (int)pidCfg2->pid);
}

void LoggerApiTest::testSetScript(){
	testSetScriptFile("setScript1.json");
}

void LoggerApiTest::testGetScript(){
	testGetScriptFile("getScript1.json");
}

void LoggerApiTest::testGetScriptFile(string filename){
	char * response = processApiGeneric(filename);

	Object json;
	stringToJson(response, json);

	CPPUNIT_ASSERT_EQUAL(string("function onTick() end"), (string)(String)json["script"]["data"]);

}

void LoggerApiTest::testSetScriptFile(string filename){

}
