#include	"../r_comp/decompiler.h"
#include	"../r_comp/compiler.h"
#include	"../r_comp/preprocessor.h"

#include	"../r_exec/Mem.h"

#include	<iostream>


using	namespace	r_comp;

int32	main(int	argc,char	**argv){

	std::string		error;
	std::ifstream	source_code(argv[1]);	//	ANSI encoding (not Unicode)

	if(!source_code.good()){

		std::cout<<"error: unable to load file "<<argv[1]<<std::endl;
		return	1;
	}

	//	for decompiling
	std::ostringstream	decompiled_code;

	//	compile the source code and show the result
	DefinitionSegment		definition_segment;	//	preprocessor output
	HardCodedPreprocessor	preprocessor;
	std::ostringstream		preprocessed_code;
	if(!preprocessor.process(&definition_segment,&source_code,&preprocessed_code,&error)){

		std::cout<<error;
		return	2;
	}
	r_comp::Image	*_image=new	r_comp::Image(&definition_segment);	//	compiler input
	r_code::Image	*image;											//	compiler output, decompiler input, r_exec::Mem input
	Compiler		compiler;
	Decompiler		decompiler;
	if(!compiler.compile(&source_code,_image,image,&error)){

		std::cout<<error;
		source_code.close();
		delete	image;
		delete	_image;
		return	3;
	}else{

		image->trace();
		source_code.close();
#if 1
		//	Loading code from the image into memory then into the r_exec::Mem
		//	Instantiate objects and views
		r_code::vector<r_code::Object	*>	ram_objects;
		uint32	i;
		for(i=0;i<_image->code_segment.objects.size();++i)
			ram_objects[i]=new	r_code::Object(_image->code_segment.objects[i]);
		//	Translate indices into pointers
		for(i=0;i<_image->relocation_segment.entries.size();++i){	//	for each allocated object, write its address in the reference set of the objects or views that reference it

			r_code::Object	*referenced_object=ram_objects[i];
			RelocationSegment::Entry	e=_image->relocation_segment.entries[i];
			for(uint32	j=0;j<e.pointer_indexes.size();++j){

				RelocationSegment::PointerIndex	p=e.pointer_indexes[j];
				r_code::Object	*referencing_object=ram_objects[p.object_index];
				if(p.view_index==-1)
					referencing_object->reference_set[p.pointer_index]=referenced_object;
				else
					referencing_object->view_set[p.view_index]->reference_set[p.pointer_index]=referenced_object;
			}
			//	- put marker addresses in each object's marker set (marker sets are subsets of reference sets)
			//	- no need to do the same for members in groups: the Mem shall do it itself
		}
		// Translate the compiler's class table to give just an Atom for use by the Mem
		UNORDERED_MAP<std::string, r_code::Atom> classes;
		UNORDERED_MAP<std::string, r_comp::Class>::iterator it;
		for (it = _image->definition_segment->classes.begin(); it != _image->definition_segment->classes.end(); ++it) {
			classes.insert(make_pair(it->first, it->second.atom));
		}
		for (it = _image->definition_segment->sys_classes.begin(); it != _image->definition_segment->sys_classes.end(); ++it) {
			classes.insert(make_pair(it->first, it->second.atom));
		}

		delete	_image;

		//	Create the mem with objects defined in ram_objects
#if 1
		r_exec::Mem* mem = r_exec::Mem::create(
			classes,
			*ram_objects.as_std(),
			0
		);
		sleep(20);
#endif
					
		//	Loading code from memory to an r_comp::Image
		r_comp::Image	*_image=new	r_comp::Image(&definition_segment);
		UNORDERED_MAP<Object	*,uint32>	ptrs_to_indices;
		for(i=0;i<ram_objects.size();++i){
		
			SysObject	*sys_object=new	SysObject(ram_objects[i],i);
			_image->addObject(sys_object);
			ptrs_to_indices[ram_objects[i]]=i;
		}
		//	Translate pointers into indices
		for(i=0;i<_image->code_segment.objects.size();++i){	//	for each sys_object, valuate the reference set and marker set, plus reference set of views

			SysObject	*sys_object=_image->code_segment.objects[i];
			uint32	j;
			for(j=0;j<ram_objects[i]->reference_set.size();++j){

				uint32	referenced_object_index=ptrs_to_indices.find(ram_objects[i]->reference_set[j])->second;
				sys_object->reference_set.push_back(referenced_object_index);
				_image->relocation_segment.addObjectReference(referenced_object_index,i,j);
			}
			for(j=0;j<ram_objects[i]->view_set.size();++j)
				for(uint32	k=0;k<ram_objects[i]->view_set[j]->reference_set.size();++k){

					uint32	referenced_object_index=ptrs_to_indices.find(ram_objects[i]->view_set[j]->reference_set[k])->second;
					sys_object->view_set[j]->reference_set.push_back(referenced_object_index);
					_image->relocation_segment.addViewReference(referenced_object_index,i,j,k);
				}
			//	for(j=0;j<ram_objects[i]->marker_set.size();++j)
			//		_image->relocation_segment.addMarkerReference(ptrs_to_indices.find(ram_objects[i]->marker_set[j])->second,i,j);
		}
#endif
		//decompiler.decompile(image,&decompiled_code);	//	this is to be called when the image comes from disk/network: not ready yet

		decompiler.decompile(_image,&decompiled_code);	//	uses the direct output form the compiler (instead of the r_code::Image): we can do that since there's no streaming of the image (disk/network), and _image is available
		std::cout<<"\n\n DECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
		delete	image;
		delete	_image;
	}

	return	0;
}
