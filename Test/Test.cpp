#include	"../r_comp/decompiler.h"
#include	"../r_comp/compiler.h"
#include	"../r_comp/preprocessor.h"

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
	r_comp::Image			*_image;	//	preprocessor output, compiler input
	HardCodedPreprocessor	preprocessor;
	std::ostringstream		preprocessed_code;
	if(!preprocessor.process(_image,&source_code,&preprocessed_code,&error)){

		std::cout<<error;
		return	2;
	}
	r_code::Image	*image;	//	compiler output, decompiler input, r_exec::Mem input
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

		//	Loading code from the image into memory then into the r_exec::Mem
		//	Instantiate objects and views
		//	r_code::vector<r_code::Object	*>	ram_objects;
		//	uint32	i;
		//	for(i=0;i<_image->code_segment.objects.size();++i){

			//	ram_objects[i]=new	r_code::Object(_image->code_segment.objects[i]);
			//	for(uint32	j=0;j<_image->code_segment.objects[i]->view_set.size();++j){

				//	ram_objects[i]->view_set[j]=new	r_code::View(image->code_segment.objects[i]->view_set[j]);
			//	}
		//	}
		//	Translate indices into pointers
		//	for(i=0;i<ram_objects.size();++i){

			//	uint32	j;
			//	for(j=0;j<ram_objects[i]->reference_set.size();++j){
			//
			//		ram_objects[i]->reference_set[j]=ram_object[ram_objects[i]->reference_set[j]];
			//	}
			//	for(j=0;j<ram_objects[i]->view_set.size();++j){
			//	
			//		for(uint32	k=0;k<ram_objects[i]->view_set[j]->reference_set.size();++k){
			//
			//			ram_objects[i]->view_set[j]->reference_set[k]=ram_object[ram_objects[i]->view_set[j]->reference_set[k]];
			//		}
			//	}
			//	for(j=0;j<ram_objects[i]->marker_set.size();++j){
			//	
			//		ram_objects[i]->marker_set[j]=ram_object[ram_objects[i]->marker_set[j]];
			//	}
			//	Also, populate group's member_set
			//	...
		//	}
		//	Load objects in the r_exec::Mem
		//	for(i=0;i<ram_objects.size();++i){

			//	r_exec::Mem::Get()->receive(&ram_objects[i]);
		//	}

		//	Loading code from memory to an r_comp::Image
		//	r_comp::Image	*_image=new	r_comp::Image();
		//	unordered_map<,uint32>	ptrs_to_indices;
		//	for(i=0;i<ram_objects.size();++i){
		//
		//		ptrs_to_indices[ram_objects[i]]=i;
		//		SysObject	*sys_object=ram_objects[i]->asSysObject();
		//		_image->addObject(sys_object);
		//	}
		//	Translate pointers into indices
		//	for(i=0;i<_image->code_segment.size();++i){
		//
		//		ptrs_to_indices[ram_objects[i]]=i;
		//		SysObject	*sys_object=r_image->code_segment[i];
		//		uint32	j;
			//	for(j=0;j<sys_object->reference_set.size();++j){
			//
			//		sys_object->reference_set[j]=ptrs_to_indices.find(r_image->code_segment[sys_object->reference_set[j]]).second;
			//	}
			//	for(j=0;j<sys_object->view_set.size();++j){
			//	
			//		for(uint32	k=0;k<sys_object->view_set[j]->reference_set.size();++k){
			//
			//			sys_object->view_set[j]->reference_set[k]=ptrs_to_indices.find(r_image->code_segment[sys_object->view_set[j]->reference_set[k]]).second;
			//		}
			//	}
			//	for(j=0;j<sys_object->marker_set.size();++j){
			//	
			//		ram_objects[i]->marker_set[j]=ptrs_to_indices.find(r_image->code_segment[sys_object->marker_set[j]]).second;
			//	}
			//	Also, translate ptrs in group's member_set into indices
			//	...
		//	}

		//	N.B.: reloc segment seems useless!

		//decompiler.decompile(image,&decompiled_code);	//	this is to be called when the image comes from disk/network: not ready yet

		decompiler.decompile(_image,&decompiled_code);	//	uses the direct output form the compiler (instead of the r_code::Image): we can do that since there's no streaming of the image (disk/network), and _image is available
		std::cout<<"\n\n DECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
		delete	image;
		delete	_image;
	}

	return	0;
}