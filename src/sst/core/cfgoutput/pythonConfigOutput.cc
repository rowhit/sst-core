// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include <sst_config.h>

#include "pythonConfigOutput.h"

using namespace SST::Core;

PythonConfigGraphOutput::PythonConfigGraphOutput(const char* path) :
	ConfigGraphOutput(path) {

}

void PythonConfigGraphOutput::generate(const Config* cfg,
	ConfigGraph* graph) throw(ConfigGraphOutputException) {

	if(NULL == outputFile) {
		throw ConfigGraphOutputException("Input file is not open for output writing");
	}

	// Generate the header and program options
	fprintf(outputFile, "# Automatically generated by SST\n");
	fprintf(outputFile, "import sst\n\n");
	fprintf(outputFile, "# Define SST Program Options:\n");
	fprintf(outputFile, "sst.setProgramOption(\"timebase\", \"%s\")\n",
		cfg->timeBase.c_str());
	fprintf(outputFile, "sst.setProgramOption(\"stopAtCycle\", \"%s\")\n\n",
		cfg->stopAtCycle.c_str());

	// Output the graph
	fprintf(outputFile, "# Define the SST Components:\n");

	auto compMap = graph->getComponentMap();
	for(auto comp_itr = compMap.begin(); comp_itr != compMap.end(); comp_itr++) {
		char* pyCompName = makePythonSafeWithPrefix(comp_itr->name.c_str(), "comp_");
		char* esCompName = makeEscapeSafe(comp_itr->name.c_str());

		fprintf(outputFile, "%s = sst.Component(\"%s\", \"%s\")\n",
			pyCompName, esCompName, comp_itr->type.c_str());

        Params& params = comp_itr->params;
        auto keys = params.getKeys();
		// auto params_itr = comp_itr->params.begin();
		auto params_itr = keys.begin();

		// if(params_itr != comp_itr->params.end()) {
		if(params_itr != keys.end()) {
			// char* esParamName = makeEscapeSafe(Params::getParamName(params_itr->first).c_str());
			// char* esValue     = makeEscapeSafe(params_itr->second.c_str());
			char* esParamName = makeEscapeSafe(params_itr->c_str());
			char* esValue     = makeEscapeSafe(params.find<std::string>(*params_itr).c_str());

			fprintf(outputFile, "%s.addParams({\n", pyCompName);

			if(isMultiLine(esValue)) {
				fprintf(outputFile, "     \"%s\" : \"\"\"%s\"\"\"", esParamName, esValue);
			} else {
				fprintf(outputFile, "     \"%s\" : \"%s\"", esParamName, esValue);
			}

			free(esParamName);
			free(esValue);

			params_itr++;

			// for(; params_itr != comp_itr->params.end(); params_itr++) {
			for(; params_itr != keys.end(); params_itr++) {
				char* esParamName = makeEscapeSafe(params_itr->c_str());
				char* esValue     = makeEscapeSafe(params.find<std::string>(*params_itr).c_str());

				if(isMultiLine(esValue)) {
					fprintf(outputFile, ",\n     \"%s\" : \"\"\"%s\"\"\"",
						esParamName, esValue);
				} else {
					fprintf(outputFile, ",\n     \"%s\" : \"%s\"",
						esParamName, esValue);
				}

				free(esParamName);
				free(esValue);
			}

			fprintf(outputFile, "\n})\n");
		}
	}

	// Dump the Component Statistics
	fprintf(outputFile, "\n\n# Define the SST Component Statistics Information\n");
	// Output statistics options
	fprintf(outputFile, "# Define SST Statistics Options:\n");

	if( 0 != graph->getStatLoadLevel() ) {
		fprintf(outputFile, "sst.setStatisticLoadLevel(%" PRIu64 ")\n",
			(uint64_t) graph->getStatLoadLevel());
	}

	fprintf(outputFile, "\n# Define Component Statistics Information:\n");
    for(auto comp_itr = compMap.begin(); comp_itr != compMap.end(); comp_itr++) {
        for(size_t statIndex = 0; statIndex < comp_itr->enabledStatistics.size(); statIndex++) {
            char* pyCompName = makePythonSafeWithPrefix(comp_itr->name.c_str(), "comp_");

            if ( comp_itr->enabledStatistics[statIndex] == STATALLFLAG ) {
                fprintf(outputFile, "%s.enableAllStatistics(", pyCompName);
                if ( !comp_itr->enabledStatParams[statIndex].empty() ) {
                    fprintf(outputFile, "{\n");

                    Params& params = comp_itr->enabledStatParams[statIndex];
                    auto keys = params.getKeys();
                    auto param_itr = keys.begin();
                    // auto param_itr = comp_itr->enabledStatParams[statIndex].begin();

                    // for(; param_itr != comp_itr->enabledStatParams[statIndex].end(); param_itr++) {
                    for(; param_itr != keys.end(); param_itr++) {
                        // if(param_itr != comp_itr->enabledStatParams[statIndex].begin()) {
                        if(param_itr != keys.begin()) {
                            fprintf(outputFile, ",\n");
                        }

                        // char* esParamName = makeEscapeSafe(Params::getParamName(param_itr->first));
                        // char* esValue     = makeEscapeSafe(param_itr->second);
                        char* esParamName = makeEscapeSafe(*param_itr);
                        char* esValue     = makeEscapeSafe(comp_itr->enabledStatParams[statIndex].find<std::string>(*param_itr));

                        fprintf(outputFile, "     \"%s\" : \"%s\"", esParamName, esValue);

                        free(esParamName);
                        free(esValue);
                    }
                    fprintf(outputFile, "}");
                }
                fprintf(outputFile, ")\n");
            } else {
                char* esStatName = makeEscapeSafe(comp_itr->enabledStatistics[statIndex].c_str());

                fprintf(outputFile, "%s.enableStatistics([\"%s\"]",
                        pyCompName, esStatName);

                // Output the Statistic Parameters
                if( 0 != comp_itr->enabledStatParams[statIndex].size() ) {
                    fprintf(outputFile, ", {\n");

                    Params& params = comp_itr->enabledStatParams[statIndex];
                    auto keys = params.getKeys();
                    auto param_itr = keys.begin();
                    // auto param_itr = comp_itr->enabledStatParams[statIndex].begin();

                    // for(; param_itr != comp_itr->enabledStatParams[statIndex].end(); param_itr++) {
                    for(; param_itr != keys.end(); param_itr++) {
                        // if(param_itr != comp_itr->enabledStatParams[statIndex].begin()) {
                        if(param_itr != keys.begin()) {
                            fprintf(outputFile, ",\n");
                        }

                        // char* esParamName = makeEscapeSafe(Params::getParamName(param_itr->first));
                        // char* esValue     = makeEscapeSafe(param_itr->second);
                        char* esParamName = makeEscapeSafe(*param_itr);
                        char* esValue     = makeEscapeSafe(comp_itr->enabledStatParams[statIndex].find<std::string>(*param_itr));

                        fprintf(outputFile, "     \"%s\" : \"%s\"", esParamName, esValue);

                        free(esParamName);
                        free(esValue);
                    }
                    fprintf(outputFile, "\n}");
                }
                fprintf(outputFile, ")\n");
                free(esStatName);
            }
            free(pyCompName);
        }
    }

	// Dump the SST Simulation Link Information
	auto linkMap = graph->getLinkMap();

	fprintf(outputFile, "\n\n# Define SST Simulation Link Information\n");
	for(auto link_itr = linkMap.begin(); link_itr != linkMap.end(); link_itr++) {
		ConfigComponent* link_left  = &compMap[link_itr->component[0]];
		ConfigComponent* link_right = &compMap[link_itr->component[1]];

		char* pyLinkName     = makePythonSafeWithPrefix(link_itr->name,  "link_");
		char* pyLinkNameStr  = makePythonSafeWithPrefix(link_itr->name, pyLinkName);

		fprintf(outputFile, "%s = sst.Link(\"%s\")\n", pyLinkName, pyLinkNameStr);

		char* pyLeftCompName  = makePythonSafeWithPrefix(link_left->name.c_str(),  "comp_");
		char* pyRightCompName = makePythonSafeWithPrefix(link_right->name.c_str(), "comp_");

		char* esLeftPortName  = makeEscapeSafe(link_itr->port[0].c_str());
		char* esRightPortName = makeEscapeSafe(link_itr->port[1].c_str());

		fprintf(outputFile, "%s.connect( (%s, \"%s\", \"%" PRIu64
			"ps\"), (%s, \"%s\", \"%" PRIu64 "ps\") )\n",
			pyLinkName, pyLeftCompName, esLeftPortName, link_itr->latency[0],
			pyRightCompName, esRightPortName, link_itr->latency[1]);

		free(pyLinkName);
		free(pyLinkNameStr);
		free(pyLeftCompName);
		free(pyRightCompName);
		free(esLeftPortName);
		free(esRightPortName);
	}

	fprintf(outputFile, "# End of generated output.\n\n");
}

bool PythonConfigGraphOutput::isMultiLine(const std::string check) const {
	bool isMultiLine = false;

	for(size_t i = 0; i < check.size(); i++) {
		if(check.at(i) == '\n' || check.at(i) == '\r' || check.at(i) == '\f') {
			isMultiLine = true;
			break;
		}
	}

	return isMultiLine;
}

bool PythonConfigGraphOutput::isMultiLine(const char* check) const {
	const int checkLen = strlen(check);
	bool isMultiLine = false;

	for(int i = 0; i < checkLen; i++) {
		if(check[i] == '\n' || check[i] == '\f' || check[i] == '\r') {
			isMultiLine = true;
			break;
		}
	}

	return isMultiLine;
}

char* PythonConfigGraphOutput::makePythonSafeWithPrefix(const std::string name,
	const std::string prefix) const {

	const auto nameLength = name.size();
	char* buffer = NULL;

	if( nameLength > 0 && isdigit(name.at(0)) ) {
		if( name.size() > prefix.size() ) {
			if( name.substr(0, prefix.size()) == prefix ) {
				buffer = (char*) malloc( sizeof(char) * (name.size() + 3) );
				sprintf(buffer, "s_%s", name.c_str());
			} else {
				buffer = (char*) malloc( sizeof(char) * (name.size() +
					prefix.size() + 3) );
				sprintf(buffer, "%ss_%s", prefix.c_str(), name.c_str());
			}
		} else {
			buffer = (char*) malloc( sizeof(char) * (name.size() + prefix.size() + 3) );
			sprintf(buffer, "%ss_%s", prefix.c_str(), name.c_str());
		}
	} else {
		if( name.size() > prefix.size() ) {
			if( name.substr(0, prefix.size()) == prefix ) {
				buffer = (char*) malloc( sizeof(char) * (name.size() + 3) );
				sprintf(buffer, "%s", name.c_str());
			} else {
				buffer = (char*) malloc( sizeof(char) * (prefix.size() + name.size() + 1) );
				sprintf(buffer, "%s%s", prefix.c_str(), name.c_str());
			}
		} else {
			buffer = (char*) malloc( sizeof(char) * (prefix.size() + name.size() + 1) );
			sprintf(buffer, "%s%s", prefix.c_str(), name.c_str());
		}
	}

	makeBufferPythonSafe(buffer);
	return buffer;
};

void PythonConfigGraphOutput::makeBufferPythonSafe(char* buffer)
	const {

	const auto length = strlen(buffer);

	for(size_t i = 0; i < length; i++) {
                switch(buffer[i]) {
                case '.':
                        buffer[i] = '_'; break;
                case ':':
                        buffer[i] = '_'; break;
                case ',':
                        buffer[i] = '_'; break;
                case '-':
                        buffer[i] = '_'; break;
                }
        }
}

bool PythonConfigGraphOutput::strncmp(const char* a, const char* b,
	const size_t n) const {

	bool matched = true;

	for(size_t i = 0; i < n; i++) {
		if(a[i] != b[i]) {
			matched = false;
			break;
		}
	}

	return matched;
};

char* PythonConfigGraphOutput::makeEscapeSafe(const std::string input) const {

	std::string escapedInput = "";
	auto inputLength = input.size();

	for(size_t i = 0; i < inputLength; i++) {
		const char nextChar = input.at(i);

		switch(nextChar) {
		case '\"':
			escapedInput = escapedInput + "\\\""; break;
		case '\'':
			escapedInput = escapedInput + "\\\'"; break;
		case '\n':
			escapedInput = escapedInput + "\\n"; break;
		default:
			escapedInput.push_back(nextChar);
		}
	}

	char* escapedBuffer = (char*) malloc( sizeof(char) * (1 + escapedInput.size()) );
	sprintf(escapedBuffer, "%s", escapedInput.c_str());

	return escapedBuffer;
}
