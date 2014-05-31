/*
 * libsf-common
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <cconfig.h>
#include "common.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <sstream>

using namespace config;


#define ROOT_ELEMENT "SENSOR"
#define TEXT_ELEMENT "text"
#define MODEL_ID_ATTR "id"



CConfig CConfig::inst;


CConfig::CConfig()
{

}

bool CConfig::load_config(const string& config_path)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	static bool load_done = false;

	if (load_done == true)
		return true;

	DBG("CConfig::load_config(\"%s\") is called!\n",config_path.c_str());

	doc = xmlParseFile(config_path.c_str());

	if (doc == NULL) {
		ERR("There is no %s\n",config_path.c_str());
		return false;
	}

	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		ERR("There is no root element in %s\n",config_path.c_str());
		xmlFreeDoc(doc);
		return false;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *)ROOT_ELEMENT)) {
		ERR("Wrong type document: there is no [%s] root element in %s\n",ROOT_ELEMENT,config_path.c_str());
		xmlFreeDoc(doc);
		return false;
	}

	xmlNodePtr model_list_node_ptr;
	xmlNodePtr model_node_ptr;
	xmlNodePtr element_node_ptr;
	xmlAttrPtr attr_ptr;
	char* prop = NULL;

	model_list_node_ptr = cur->xmlChildrenNode;

	while (model_list_node_ptr != NULL) {
		//skip garbage element, [text]
		if (!xmlStrcmp(model_list_node_ptr->name,(const xmlChar *)TEXT_ELEMENT)) {
			model_list_node_ptr = model_list_node_ptr->next;
			continue;
		}

		//insert Model_list to config map
		m_sensor_config[(const char*)model_list_node_ptr->name];
		DBG("<%s>\n",(const char*)model_list_node_ptr->name);

		model_node_ptr = model_list_node_ptr->xmlChildrenNode;
		while (model_node_ptr != NULL){
			//skip garbage element, [text]
			if (!xmlStrcmp(model_node_ptr->name,(const xmlChar *)TEXT_ELEMENT)) {
				model_node_ptr = model_node_ptr->next;
				continue;
			}


			string model_id;
			prop = (char*)xmlGetProp(model_node_ptr,(const xmlChar*)MODEL_ID_ATTR);
			model_id = prop;
			free(prop);

			//insert Model to Model_list
			m_sensor_config[(const char*)model_list_node_ptr->name][model_id];
			DBG("<%s id=\"%s\">\n",(const char*)model_list_node_ptr->name,model_id.c_str());

			element_node_ptr = model_node_ptr->xmlChildrenNode;
			while (element_node_ptr != NULL) {
				//skip garbage element, [text]
				if (!xmlStrcmp(element_node_ptr->name,(const xmlChar *)TEXT_ELEMENT)) {
					element_node_ptr = element_node_ptr->next;
					continue;
				}

				//insert Element to Model
				m_sensor_config[(const char*)model_list_node_ptr->name][model_id][(const char*)element_node_ptr->name];
				DBG("<%s id=\"%s\"><%s>\n",(const char*)model_list_node_ptr->name,model_id.c_str(),(const char*)element_node_ptr->name);

				attr_ptr = element_node_ptr->properties;
				while (attr_ptr != NULL) {

					string key,value;
					key = (char*)attr_ptr->name;
					prop = (char*)xmlGetProp(element_node_ptr,attr_ptr->name);
					value = prop;
					free(prop);

					//insert attribute to Element
					m_sensor_config[(const char*)model_list_node_ptr->name][model_id][(const char*)element_node_ptr->name][key]=value;
					DBG("<%s id=\"%s\"><%s \"%s\"=\"%s\">\n",(const char*)model_list_node_ptr->name,model_id.c_str(),(const char*)element_node_ptr->name,key.c_str(),value.c_str());
					attr_ptr = attr_ptr->next;
				}


				element_node_ptr = element_node_ptr->next;
			}

			DBG("\n");
			model_node_ptr = model_node_ptr->next;
		}

		DBG("\n");
		model_list_node_ptr = model_list_node_ptr->next;
	}

	xmlFreeDoc(doc);

	load_done = true;
	return true;
}


bool CConfig::get(const string& sensor_type,const string& model_id, const string& element, const string& attr, string& value)
{

	load_config();

	Sensor_config::iterator it_model_list;

	it_model_list = m_sensor_config.find(sensor_type);

	if (it_model_list == m_sensor_config.end())	{
		ERR("There is no <%s> element\n",sensor_type.c_str());
		return false;
	}

	Model_list::iterator it_model;

	it_model = it_model_list->second.find(model_id);

	if (it_model == it_model_list->second.end()) {
		ERR("There is no <%s id=\"%s\"> element\n",sensor_type.c_str(),model_id.c_str());
		return false;
	}

	Model::iterator it_element;

	it_element = it_model->second.find(element);

	if (it_element == it_model->second.end()) {
		ERR("There is no <%s id=\"%s\"><%s> element\n",sensor_type.c_str(),model_id.c_str(),element.c_str());
		return false;
	}

	Element::iterator it_attr;

	it_attr = it_element->second.find(attr);

	if (it_attr == it_element->second.end()) {
		ERR("There is no <%s id=\"%s\"><%s \"%s\">\n",sensor_type.c_str(),model_id.c_str(),element.c_str(),attr.c_str());
		return false;
	}
	value = it_attr->second;

	return true;
}


bool CConfig::get(const string& sensor_type, const string& model_id, const string& element, const string& attr, double& value)
{
	load_config();

	string str_value;

	if ( get(sensor_type,model_id,element,attr,str_value) == false)
		return false;

	istringstream convert(str_value);

	if ( !(convert >> value))
		value = 0;

	return true;


}
bool CConfig::get(const string& sensor_type, const string& model_id, const string& element, const string& attr, long& value)
{
	load_config();

	string str_value;

	if ( get(sensor_type,model_id,element,attr,str_value) == false)
		return false;

	istringstream convert(str_value);

	if ( !(convert >> value))
		value = 0;

	return true;
}


bool CConfig::is_supported(const string& sensor_type,const string& model_id)
{
	load_config();

	Sensor_config::iterator it_model_list;

	it_model_list = m_sensor_config.find(sensor_type);

	if (it_model_list == m_sensor_config.end())
		return false;

	Model_list::iterator it_model;

	it_model = it_model_list->second.find(model_id);

	if (it_model == it_model_list->second.end())
		return false;

	return true;
}
