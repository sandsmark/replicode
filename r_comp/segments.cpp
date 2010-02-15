#include	"segments.h"

#include	<iostream>


namespace	r_comp{

	Reference::Reference(){
	}

	Reference::Reference(const	uint16	i,const	Class	&c):index(i),_class(c){
	}

	////////////////////////////////////////////////////////////////

	DefinitionSegment::DefinitionSegment(){
	}

	Class	*DefinitionSegment::getClass(std::string	&class_name){

		UNORDERED_MAP<std::string,Class>::iterator	it=classes.find(class_name);
		if(it!=classes.end())
			return	&it->second;
		return	NULL;
	}

	Class	*DefinitionSegment::getClass(uint16	opcode){

		return	&classes_by_opcodes[opcode];
	}
	
	void	DefinitionSegment::write(word32	*data){
		//	TODO
	}
		
	void	DefinitionSegment::read(word32	*data,uint32	size){
		//	TODO
	}

	uint32	DefinitionSegment::getSize(){

		return	0;	//	TODO
	}

	////////////////////////////////////////////////////////////////

	void	ObjectMap::shift(uint32	offset){

		for(uint32	i=0;i<objects.size();++i)
			objects[i]+=offset;
	}

	void	ObjectMap::write(word32	*data){

		for(uint32	i=0;i<objects.size();++i)
			data[i]=objects[i];
	}

	void	ObjectMap::read(word32	*data,uint32	size){

		for(uint32	i=0;i<size;++i)
			objects.push_back(data[i]);
	}

	uint32	ObjectMap::getSize()	const{

		return	objects.size();
	}

	////////////////////////////////////////////////////////////////

	CodeSegment::~CodeSegment(){

		for(uint32	i=0;i<objects.size();++i)
			delete	objects[i];
	}

	void	CodeSegment::write(word32	*data){

		uint32	offset=0;
		for(uint32	i=0;i<objects.size();++i){

			objects[i]->write(data+offset);
			offset+=objects[i]->getSize();
		}
	}

	void	CodeSegment::read(word32	*data,uint32	object_count){

		uint32	offset=0;
		for(uint32	i=0;i<object_count;++i){

			SysObject	*o=new	SysObject();
			o->read(data+offset);
			objects.push_back(o);
			offset+=o->getSize();
		}
	}

	uint32	CodeSegment::getSize(){

		uint32	size=0;
		for(uint32	i=0;i<objects.size();++i)
			size+=objects[i]->getSize();
		return	size;
	}

	////////////////////////////////////////////////////////////////

	RelocationSegment::PointerIndex::PointerIndex():object_index(0),pointer_index(0){
	}

	RelocationSegment::PointerIndex::PointerIndex(uint32	object_index,int32	view_index,uint32	pointer_index):object_index(object_index),view_index(view_index),pointer_index(pointer_index){
	}

	void	RelocationSegment::Entry::write(word32	*data){

		data[0]=pointer_indexes.size();
		for(uint32	i=0;i<pointer_indexes.size();++i){

			data[1+i*3]=pointer_indexes[i].object_index;
			data[2+i*3]=pointer_indexes[i].view_index;
			data[3+i*3]=pointer_indexes[i].pointer_index;
		}
	}

	void	RelocationSegment::Entry::read(word32	*data){

		for(uint32	i=0;i<(uint32)data[0];++i)
			pointer_indexes.push_back(PointerIndex(data[1+i*3],data[2+i*3],data[3+i*3]));
	}

	uint32	RelocationSegment::Entry::getSize()	const{

		return	1+pointer_indexes.size()*3;
	}

	void	RelocationSegment::addObjectReference(uint32	referenced_object_index,uint32	referencing_object_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,-1,reference_pointer_index));
	}

	void	RelocationSegment::addViewReference(uint32	referenced_object_index,uint32	referencing_object_index,int32	referencing_view_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,referencing_view_index,reference_pointer_index));
	}

	void	RelocationSegment::addMarkerReference(uint32	referenced_object_index,uint32	referencing_object_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,-2,reference_pointer_index));
	}
	
	void	RelocationSegment::write(word32	*data){

		data[0]=entries.size();
		uint32	offset=0;
		for(uint32	i=0;i<entries.size();++i){

			entries[i].write(data+1+offset);
			offset+=entries[i].getSize();
		}
	}

	void	RelocationSegment::read(word32	*data){

		uint32	entry_count=data[0];
		uint32	offset=0;
		for(uint32	i=0;i<entry_count;++i){

			Entry	e;
			e.read(data+offset);
			entries.push_back(e);
			offset+=e.getSize();
		}
	}

	uint32	RelocationSegment::getSize(){

		uint32	size=1;
		for(uint32	i=0;i<entries.size();++i)
			size+=entries[i].getSize();
		return	size;
	}

	////////////////////////////////////////////////////////////////

	Image::Image():map_offset(0){
	}

	void	Image::write(r_code::Image *image){

		image->def_size=definition_segment.getSize();
		image->map_size=object_map.getSize();
		image->code_size=code_segment.getSize();
		image->reloc_size=relocation_segment.getSize();

		image->data=new	word32[image->def_size+image->map_size+image->code_size+image->reloc_size];

		object_map.shift(image->def_size+image->map_size);

		definition_segment.write(image->data);
		object_map.write(image->data+image->def_size);
		code_segment.write(image->data+image->def_size+image->map_size);
		relocation_segment.write(image->data+image->def_size+image->map_size+image->code_size);
	}

	void	Image::read(r_code::Image *image){

		definition_segment.read(image->data,image->def_size);
		object_map.read(image->data+image->def_size,image->map_size);
		code_segment.read(image->data+image->def_size+image->map_size,image->map_size);
		relocation_segment.read(image->data+image->def_size+image->map_size+image->code_size);
	}

	void	Image::addObject(SysObject	*object){

		code_segment.objects.push_back(object);
		object_map.objects.push_back(map_offset);
		map_offset+=object->getSize();
	}
}