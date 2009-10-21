#include	"precompiler.h"

namespace	replicode{

UNORDERED_MAP<std::string,RepliMacro*> RepliStruct::repliMacros;
UNORDERED_MAP<std::string,int> RepliStruct::counters;
std::list<RepliCondition*> RepliStruct::conditions;
unsigned int RepliStruct::globalLine = 1;


RepliStruct::RepliStruct(RepliStruct::Type type) {
	this->type = type;
	line = globalLine;
	parent = NULL;
}

RepliStruct::~RepliStruct() {
	parent = NULL;
}

unsigned int RepliStruct::getIndent(std::istream *stream) {
	unsigned int count = 0;
	while (!stream->eof()) {
		switch(stream->get()) {
			case 10:
				globalLine++;
			case 13:
				stream->seekg(-1, std::ios_base::cur);
				return (count/3);
			case ' ':
				count++;
				break;
			default:
				stream->seekg(-1, std::ios_base::cur);
				return (count/3);
		}
	}
	return (count/3);
}

int RepliStruct::parse(std::istream *stream, unsigned int &curIndent,
					   unsigned int &prevIndent, int paramExpect) {

	char c = 0, lastc;
	std::string str, label;
	RepliStruct* subStruct;

	int paramCount = 0;
	int returnIndent = 0;

	bool inComment = false;
	bool expectSet = false;

	while (!stream->eof()) {
		lastc = c;
		c = stream->get();
	//	printf("%c", c);
		switch(c) {
			case '\t':
				if (inComment) continue; // allow tabs in comments, does not matter anyway
				line = globalLine;
				error += "Tabs chars are not permitted. ";
				return -1;
			case '!':
				if (inComment) continue;
				if (this->type == Root) {
					subStruct = new RepliStruct(Directive);
					subStruct->parent = this;
					args.push_back(subStruct);
					if (!subStruct->parseDirective(stream, curIndent, prevIndent))
						return -1;
				}
				else {
					error += "Directive not allowed inside a structure. ";
					return -1;
				}
				break;
			case 10:
				globalLine++;
			case 13:
				// remain inComment?
				if (inComment) {
					if (lastc == ' ')
						continue; // continue comment on next line
					else
						inComment = false; // end comment
				}
				// skip all CR and LF
				while ((!stream->eof()) && (stream->peek() < 32))
					if (stream->get() == 10)
						globalLine++;
				// get indents
				prevIndent = curIndent;
				curIndent = getIndent(stream);
				// Are we in a parenthesis or set
				if (curIndent > prevIndent) {
					// Are we in a set
					if (expectSet) {
						// Add new sub structure
						subStruct = new RepliStruct(Set);
						subStruct->parent = this;
						subStruct->label = label;
						label = "";
						args.push_back(subStruct);
						returnIndent = subStruct->parse(stream, curIndent, prevIndent);
						expectSet = false;
						if (returnIndent < 0)
							return -1;
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
						if (returnIndent > 0)
							return (returnIndent - 1);
					}
					// or a parenthesis
					else {
						subStruct = new RepliStruct(Structure);
						args.push_back(subStruct);
						returnIndent = subStruct->parse(stream, curIndent, prevIndent);
						expectSet = false;
						if (returnIndent < 0)
							return -1;
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
						if (returnIndent > 0)
							return (returnIndent - 1);
					}
				}
				else if (curIndent < prevIndent) {
					if (str.size() > 0) {
						if ((cmd.size() > 0) || (type == Set)) {
							subStruct = new RepliStruct(Atom);
							subStruct->parent = this;
							args.push_back(subStruct);
							subStruct->cmd = str;
							str = "";
							if ((paramExpect > 0) && (++paramCount == paramExpect))
								return 0;
						}
						else {
							cmd = str;
							str = "";
						}
					}
					// current structure or set is complete
					return prevIndent - curIndent - 1;
				}
				else {
					// act as if we met a space
					if (str.size() > 0) {
						if ((cmd.size() > 0) || (type == Set)) {
							subStruct = new RepliStruct(Atom);
							subStruct->parent = this;
							args.push_back(subStruct);
							subStruct->cmd = str;
							str = "";
							if ((paramExpect > 0) && (++paramCount == paramExpect))
								return 0;
						}
						else {
							cmd = str;
							str = "";
						}
					}
				}
				break;
			case ';':
				inComment = true;
				break;
			case ' ':
				if (inComment) continue;
				// next string is ready
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				break;
			case '(':
				if (inComment) continue;
				// Check for scenario 'xxx('
				if (str.size() > 0) {
					if (lastc == ':') { // label:(xxx)
						label = str;
						str = "";
					}
					else if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				subStruct = new RepliStruct(Structure);
				subStruct->label = label;
				label = "";
				subStruct->parent = this;
				args.push_back(subStruct);
				returnIndent = subStruct->parse(stream, curIndent, prevIndent);
				if (returnIndent < 0)
					return -1;
				if ((paramExpect > 0) && (++paramCount == paramExpect))
					return 0;
				if (returnIndent > 0)
					return (returnIndent - 1);
				break;
			case ')':
				if (inComment) continue;
				// Check for directive use of xxx):xxx or xxx):
				if (stream->peek() == ':') {
					// expect ':' or ':xxx'
					while ( (!stream->eof()) && ((c = stream->get()) > 32) )
						tail += c;
				}
				// We met a boundary, act as ' '
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				return 0;
			case '{':
				if (inComment) continue;
				// Check for scenario 'xxx{'
				if (str.size() > 0) {
					if (lastc == ':') { // label:{xxx}
						label = str;
						str = "";
					}
					else if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				subStruct = new RepliStruct(Development);
				subStruct->label = label;
				label = "";
				subStruct->parent = this;
				args.push_back(subStruct);
				returnIndent = subStruct->parse(stream, curIndent, prevIndent);
				if (returnIndent < 0)
					return -1;
				if ((paramExpect > 0) && (++paramCount == paramExpect))
					return 0;
				if (returnIndent > 0)
					return (returnIndent - 1);
				break;
			case '}':
				if (inComment) continue;
				// Check for directive use of xxx):xxx or xxx):
				if (stream->peek() == ':') {
					// expect ':' or ':xxx'
					while ( (!stream->eof()) && ((c = stream->get()) > 32) )
						tail += c;
				}
				// We met a boundary, act as ' '
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				return 0;
			case '|':
				if (inComment) continue;
				if (stream->peek() == '[') {
					stream->get(); // read the [
					stream->get(); // read the ]
					str += "|[]";
				}
				else
					str += c;
				break;
			case '[': // set start
				if (inComment) continue;
				if (lastc == ':') { // label:[xxx]
					label = str;
					str = "";
				}
				if (stream->peek() == ']') {
					stream->get(); // read the ]
					// if we have a <CR> or <LF> next we may have a line indent set
					if (stream->peek() < 32)
						expectSet = true;
					else {
						// this could be a xxx:[] or xxx[]
						if ((lastc != ':') && (lastc > 32)) { // label[]
							// act as if [] is part of the string and continue
							str += "[]";
							continue;
						}
						// create empty set
						subStruct = new RepliStruct(Set);
						subStruct->parent = this;
						subStruct->label = label;
						args.push_back(subStruct);
					}
				}
				else {
					// Check for scenario 'xxx['
					if (str.size() > 0) {
						if ((cmd.size() > 0) || (type == Set)) {
							subStruct = new RepliStruct(Atom);
							subStruct->parent = this;
							args.push_back(subStruct);
							subStruct->cmd = str;
							str = "";
							if ((paramExpect > 0) && (++paramCount == paramExpect))
								return 0;
						}
						else {
							cmd = str;
							str = "";
						}
					}
					subStruct = new RepliStruct(Set);
					subStruct->parent = this;
					subStruct->label = label;
					label = "";
					args.push_back(subStruct);
					returnIndent = subStruct->parse(stream, curIndent, prevIndent);
					if (returnIndent < 0)
						return -1;
					if ((paramExpect > 0) && (++paramCount == paramExpect))
						return 0;
					if (returnIndent > 0)
						return (returnIndent - 1);
				}
				break;
			case ']':
				if (inComment) continue;
				// We met a boundary, act as ' '
				if (str.size() > 0) {
					if ((cmd.size() > 0) || (type == Set)) {
						subStruct = new RepliStruct(Atom);
						subStruct->parent = this;
						args.push_back(subStruct);
						subStruct->cmd = str;
						str = "";
						if ((paramExpect > 0) && (++paramCount == paramExpect))
							return 0;
					}
					else {
						cmd = str;
						str = "";
					}
				}
				return 0;
			default:
				if (inComment) continue;
				str += c;
				break;
		}
	}
	return 0;
}


bool RepliStruct::parseDirective(std::istream *stream, unsigned int &curIndent, unsigned int &prevIndent) {

	std::string str = "!";
//	RepliStruct* subStruct;
	char c;

	// We know that parent read a '!', so first find out which directive
	while ( (!stream->eof()) && ((c = stream->peek()) > 32) )
		str += stream->get();
	if (stream->eof()) {
		error += "Error in directive formatting, end of file reached unexpectedly. ";
		return false;
	}

	unsigned int paramCount = 0;

	if (str.compare("!def") == 0) // () ()
		paramCount = 2;
	else if (str.compare("!counter") == 0) // xxx val
		paramCount = 2;
	else if (str.compare("!undef") == 0) // xxx
		paramCount = 1;
	else if (str.compare("!ifdef") == 0) { // name
		this->type = Condition;
		paramCount = 1;
	}
	else if (str.compare("!ifundef") == 0) { // name
		this->type = Condition;
		paramCount = 1;
	}
	else if (str.compare("!else") == 0) { //
		this->type = Condition;
		paramCount = 0;
	}
	else if (str.compare("!endif") == 0) { //
		this->type = Condition;
		paramCount = 0;
	}
	else if (str.compare("!class") == 0) // ()
		paramCount = 1;
	else if (str.compare("!op") == 0) // ():xxx
		paramCount = 1;
	else if (str.compare("!dfn") == 0) // ()
		paramCount = 1;
	else if (str.compare("!load") == 0) // xxx
		paramCount = 1;
	else {
		error += "Unknown directive: '" + str + "'. ";
		return false;
	}
	cmd = str;

	if (paramCount == 0) {
		// read until end of line, including any comments
		while ( (!stream->eof()) && (stream->peek() > 13) )
			stream->get();
		// read the end of line too
		while ( (!stream->eof()) && (stream->peek() < 32) )
			if (stream->get() == 10)
				globalLine++;
		return true;
	}

	if (parse(stream, curIndent, prevIndent, paramCount) != 0) {
		error += "Error parsing the arguments for directive '" + cmd + "'. ";
		return false;
	}
	else
		return true;
}


int RepliStruct::process() {

	int changes = 0, count;
	RepliStruct *structure, *newStruct, *tempStruct;
	RepliMacro* macro;
	RepliCondition* cond;
	std::string loadError;

	if (args.size() == 0) {
		// expand counters in all structures
		if (counters.find(cmd) != counters.end()) {
			// expand the counter
			cmd = StringUtils::Int2String(counters[cmd]++);
			changes++;
		}
		if (repliMacros.find(cmd) != repliMacros.end()) {
			// expand the macro
			macro = repliMacros[cmd];
			newStruct = macro->expandMacro(this);
			if (newStruct != NULL) {
				*this = *newStruct;
				delete(newStruct);
				changes++;
			}
			else {
				error = macro->error;
				macro->error = "";
				return -1;
			}
		}
		return changes;
	}

	for (std::list<RepliStruct*>::iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter) {
		structure = (*iter);

		// printf("Processing %s with %d args...\n", structure->cmd.c_str(), structure->args.size());
		if (structure->type == Condition) {
			if (structure->cmd.compare("!ifdef") == 0) {
				cond = new RepliCondition(structure->args.front()->cmd, false);
				conditions.push_back(cond);
			}
			else if (structure->cmd.compare("!ifundef") == 0) {
				cond = new RepliCondition(structure->args.front()->cmd, true);
				conditions.push_back(cond);
			}
			else if (structure->cmd.compare("!else") == 0) {
				// reverse the current condition
				conditions.back()->reverse();
			}
			else if (structure->cmd.compare("!endif") == 0) {
				conditions.pop_back();
			}
			return 0;
		}

		// Check conditions to see if we are active at the moment
		for (std::list<RepliCondition*>::const_iterator iCon(conditions.begin()), iConEnd(conditions.end()); iCon != iConEnd; ++iCon) {
			// if just one active condition is not active we will ignore the current line
			// until we get an !else or !endif
			if (!((*iCon)->isActive(repliMacros, counters)))
				return 0;
		}

		if (structure->type == Directive) {
			if (structure->cmd.compare("!counter") == 0) {
				// register the counter
				if (structure->args.size() > 1)
					counters[structure->args.front()->cmd] = atoi(structure->args.back()->cmd.c_str());
				else
					counters[structure->args.front()->cmd] = 0;
			}
			else if (structure->cmd.compare("!def") == 0) {
				// check second sub structure only containing macros
				while ( (count = structure->args.back()->process()) > 0 )
					changes += count;
				if (count < 0)
					return -1;
				// register the macro
				macro = new RepliMacro(structure->args.front()->cmd, structure->args.front(), structure->args.back());
				repliMacros[macro->name] = macro;
			}
			else if (structure->cmd.compare("!undef") == 0) {
				// remove the counter or macro
				repliMacros.erase(repliMacros.find(structure->args.front()->cmd));
				counters.erase(counters.find(structure->args.front()->cmd));
			}
			else if (structure->cmd.compare("!load") == 0) {
				// Check for a load directive...
				newStruct = loadReplicodeFile(structure->args.front()->cmd);
				if (newStruct == NULL) {
					structure->error += "Load: File '" + structure->args.front()->cmd + "' cannot be read! ";
					return -1;
				}
				else if ( (loadError = newStruct->printError()).size() > 0 ) {
					structure->error = loadError;
					delete(newStruct);
					return -1;
				}
				// Insert new data into current args
				// save location
				tempStruct = (*iter); 
				// insert new structures 
				args.insert(++iter, newStruct->args.begin(), newStruct->args.end());
				// reinit iterator and find location again
				iter = args.begin();
				iterEnd = args.end();
				while ((*iter) != tempStruct) iter++;
				// we want to remove the !load line, so get the next line
				iter++;
				args.remove(tempStruct);
				// and because we changed the list, repeat
				tempStruct = (*iter); 
				iter = args.begin();
				iterEnd = args.end();
				while ((*iter) != tempStruct) iter++;
				// now we have replaced the !load line with the loaded lines
				changes++;
			}
		}
		else { // a Structure, Set, Atom or Development
			if (repliMacros.find(structure->cmd) != repliMacros.end()) {
				// expand the macro
				macro = repliMacros[structure->cmd];
				newStruct = macro->expandMacro(structure);
				if (newStruct != NULL) {
					*structure = *newStruct;
					delete(newStruct);
					changes++;
				}
				else {
					structure->error = macro->error;
					macro->error = "";
					return -1;
				}
			}

			// check for sub structures containing macros
			for (std::list<RepliStruct*>::iterator iter2(structure->args.begin()), iter2End(structure->args.end()); iter2 != iter2End; ++iter2) {
				if ( (count = (*iter2)->process()) > 0 )
					changes += count;
				else if (count < 0)
					return -1;
			}
		}

		// expand counters in all structures
		if (counters.find(structure->cmd) != counters.end()) {
			// expand the counter
			structure->cmd = StringUtils::Int2String(counters[structure->cmd]++);
			changes++;
		}
	}
	
	return changes;
}

RepliStruct* RepliStruct::loadReplicodeFile(const std::string &filename) {
	RepliStruct* newRoot = new RepliStruct(Root);
	std::ifstream loadStream(filename.c_str());
	if (loadStream.bad() || loadStream.fail() || loadStream.eof()) {
		newRoot->error += "Load: File '" + filename + "' cannot be read! ";
		loadStream.close();
		return newRoot;
	}
	// create new Root structure
	unsigned int a = 0, b = 0;
	if (newRoot->parse(&loadStream, a, b) < 0) {
		// error is already recorded in newRoot
	}
	if (!loadStream.eof())
		newRoot->error = "Code structure error: Unmatched ) or ].\n";
	loadStream.close();
	return newRoot;
}


std::string RepliStruct::print() const {
	std::string str;
	switch (type) {
		case Atom:
			return cmd;
		case Structure:
		case Development:
		case Directive:
		case Condition:
			str = cmd;
			for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
				str += " " + (*iter)->print();
			if (type == Structure)
				return label + "(" + str + ")" + tail;
			else if (type == Development)
				return label + "{" + str + "}" + tail;
			else
				return str;
		case Set:
			for (std::list<RepliStruct*>::const_iterator iter(args.begin()), last(args.end()), iterEnd(last--); iter != iterEnd; ++iter) {
				str += (*iter)->print();
				if (iter != last)
					str += " ";
			}
			return label + "[" + str + "]" + tail;
		case Root:
			for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
				str += (*iter)->print() + "\n";
			return str;
		default:
			break;
	}
	return str;
}

std::ostream& operator<<(std::ostream& os, RepliStruct* structure) {
	return operator<<(os, *structure);
}

std::ostream& operator<<(std::ostream& os, const RepliStruct& structure) {
	switch (structure.type) {
		case RepliStruct::Atom:
			return os << structure.cmd;
		case RepliStruct::Directive:
		case RepliStruct::Condition:
			return os;
		case RepliStruct::Structure:
		case RepliStruct::Development:
			if (structure.type == RepliStruct::Structure)
				os << structure.label << "(";
			else if (structure.type == RepliStruct::Development)
				os << structure.label << "{";
			os << structure.cmd;
			for (std::list<RepliStruct*>::const_iterator iter(structure.args.begin()), iterEnd(structure.args.end()); iter != iterEnd; ++iter)
				os << " " << (*iter);
			if (structure.type == RepliStruct::Structure)
				os << ")" << structure.tail;
			else if (structure.type == RepliStruct::Development)
				os << "}" << structure.tail;
			break;
		case RepliStruct::Set:
			os << structure.label << "[";
			if (structure.args.size() > 0) {
				for (std::list<RepliStruct*>::const_iterator iter(structure.args.begin()), last(structure.args.end()), iterEnd(last--); iter != iterEnd; ++iter) {
					os << (*iter);
					if (iter != last)
						os << " ";
				}
			}
			os << "]" << structure.tail;
			break;
		case RepliStruct::Root:
			for (std::list<RepliStruct*>::const_iterator iter(structure.args.begin()), iterEnd(structure.args.end()); iter != iterEnd; ++iter)
				os << (*iter) << std::endl;
			break;
		default:
			break;
	}
    return os;
}

RepliStruct* RepliStruct::clone() const {
	RepliStruct* newStruct = new RepliStruct(type);
	newStruct->cmd = cmd;
	newStruct->label = label;
	newStruct->tail = tail;
	newStruct->parent = parent;
	newStruct->error = error;
	newStruct->line = line;
	for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
		newStruct->args.push_back((*iter)->clone());
	return newStruct;
}

std::string RepliStruct::printError() const {
	std::stringstream strError;
	if (error.size() > 0) {
		std::string com = cmd;
		RepliStruct* structure = parent;
		while ( (cmd.size() == 0) && (structure != NULL) ) {
			com = structure->cmd;
			structure = structure->parent;
		}
		if (com.size() == 0)
			strError << "Error";
		else
			strError << "Error in structure '" << com << "'";
		if (line > 0)
			strError << " line " << line;
		strError << ": " << error << std::endl;
	}
	for (std::list<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
		strError << (*iter)->printError();
	return strError.str();
}













RepliMacro::RepliMacro(const std::string &name, RepliStruct* src, RepliStruct* dest) {
	this->name = name;
	this->src = src;
	this->dest = dest;
}

RepliMacro::~RepliMacro() {
	name = "";
	src = NULL;
	dest = NULL;
}

unsigned int RepliMacro::argCount() {
	if (src == NULL)
		return 0;
	return src->args.size();
}

RepliStruct* RepliMacro::expandMacro(RepliStruct* oldStruct) {
	if (src == NULL) {
		error += "Macro '" + name + "' source not defined. ";
		return NULL;
	}
	if (dest == NULL) {
		error += "Macro '" + name + "' destination not defined. ";
		return NULL;
	}
	if (oldStruct == NULL) {
		error += "Macro '" + name + "' cannot expand empty structure. ";
		return NULL;
	}
	if (oldStruct->cmd.compare(this->name) != 0) {
		error += "Macro '" + name + "' cannot expand structure with different name '" + oldStruct->cmd + "'. ";
		return NULL;
	}

	if ( (src->args.size() > 0) && (src->args.size() != oldStruct->args.size()) ) {
		error += "Macro '" + name + "' requires " + StringUtils::Int2String(src->args.size()) + " arguments, cannot expand structure with " + StringUtils::Int2String(oldStruct->args.size()) + " arguments. ";
		return NULL;
	}

	RepliStruct* newStruct;

	// Special case of macros without args, just copy in the args from oldStruct
	if ((src->args.size() == 0) && (oldStruct->args.size() > 0) ) {
		newStruct = oldStruct->clone();
		newStruct->cmd = dest->cmd;
		return newStruct;
	}
	else
		newStruct = dest->clone();

	RepliStruct *findStruct;

	std::list<RepliStruct*>::const_iterator iOld(oldStruct->args.begin());
	for (std::list<RepliStruct*>::const_iterator iSrc(src->args.begin()), iSrcEnd(src->args.end()); iSrc != iSrcEnd; ++iSrc, ++iOld) {
		// printf("looking for '%s'\n", (*iSrc)->cmd.c_str());
		// find the Atom inside newStruct with the name of iSrc->cmd
		findStruct = newStruct->findAtom((*iSrc)->cmd);
		if (findStruct != NULL) {
			// overwrite data in findStruct with the matching one from old
			*findStruct = *(*iOld);
		}
	}
	
	return newStruct;
}

RepliStruct* RepliStruct::findAtom(const std::string &name) {
	RepliStruct* structure;
	for (std::list<RepliStruct*>::iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter) {
		switch ((*iter)->type) {
			case Atom:
				if ((*iter)->cmd.compare(name) == 0)
					return (*iter);
				break;
			case Structure:
			case Set:
			case Development:
				structure = (*iter)->findAtom(name);
				if (structure != NULL)
					return structure;
				break;
			default:
				break;
		}
	}
	return NULL;
}








RepliCondition::RepliCondition(const std::string &name, bool reversed) {
	this->name = name;
	this->reversed = reversed;
}

RepliCondition::~RepliCondition() {}

bool RepliCondition::reverse() {
	reversed = !reversed;
	return true;
}

bool RepliCondition::isActive(UNORDERED_MAP<std::string,RepliMacro*> &repliMacros, UNORDERED_MAP<std::string,int> &counters) {

	bool foundIt = (repliMacros.find(name) != repliMacros.end());

	if (!foundIt)
		foundIt = (counters.find(name) != counters.end());

	if (reversed)
		return (!foundIt);
	else
		return foundIt;
}















PreCompiler::PreCompiler() {
	root = NULL;
}

PreCompiler::~PreCompiler() {
	delete(root);
	root = NULL;
}

bool PreCompiler::preCompile(std::istream *stream, std::ostringstream *outstream, std::string *error) {

	RepliStruct* root = new RepliStruct(RepliStruct::Root);
	unsigned int a = 0, b = 0;
	if (root->parse(stream, a, b) < 0) {
		*error = root->printError();
		return false;
	}
	if (!stream->eof()) {
		*error = "Code structure error: Unmatched ) or ].\n";
		return false;
	}

//	printf("Replicode:\n\n%s\n", root->print().c_str());

	int pass = 0, total = 0, count;
	while ( (count = root->process()) > 0) {
		total += count;
		pass++;
	//	printf("Pass %d, %d changes, %d total\n", pass, count, total);
	}
	if (count < 0) {
		*error = root->printError();
		return false;
	}
	//printf("Replicode:\n\n%s\n", root->print().c_str());

	*outstream << root;

	printf("Replicode:\n\n%s\n", outstream->str().c_str());

	*error = root->printError();
	return (error->size() == 0);
}





}