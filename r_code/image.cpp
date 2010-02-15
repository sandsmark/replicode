#include	"image.h"

#include	<iostream>
#include	"atom.h"


namespace	r_code{

	Image::Image():def_size(0),map_size(0),code_size(0),reloc_size(0),data(NULL){
	}

	Image::~Image(){

		if(data)
			delete[]	data;
	}

	word32	*Image::getDefSegment()	const{

		return	data;
	}

	uint32	Image::getDefSegmentSize()	const{

		return	def_size;
	}

	uint32	Image::getSize()	const{

		return	def_size+map_size+code_size+reloc_size;
	}

	uint32	Image::getObjectCount()	const{

		return	map_size;
	}

	word32	*Image::getObject(uint32	i)	const{

		return	data+data[def_size+i];
	}

	word32	*Image::getCodeSegment()	const{
	
		return	data+def_size+map_size;
	}

	uint32	Image::getCodeSegmentSize()	const{

		return	code_size;
	}

	word32	*Image::getRelocSegment()	const{

		return	data+def_size+map_size+code_size;
	}

	uint32	Image::getRelocSegmentSize()	const{

		return	reloc_size;
	}

	word32	*Image::getRelocEntry(uint32	index)	const{

		word32	*reloc_segment=getRelocSegment();
		for(uint32	i=0;i<getRelocSegmentSize();++i){

			if(i==index)
				return	reloc_segment+i;	//	points to the first word of the entry, i.e. the index of the object/command in the code segment
			i+=1+reloc_segment[i];			//	points to the next entry
		}

		return	NULL;
	}

	void	Image::read(ifstream &stream){

		stream>>def_size;
		stream>>map_size;
		stream>>code_size;
		stream>>reloc_size;
		data=new	word32[getSize()];
		for(uint32	i=0;i<getSize();++i)
			stream>>data[i];
	}

	void	Image::write(ofstream &stream){

		stream<<def_size;
		stream<<map_size;
		stream<<code_size;
		stream<<reloc_size;
		for(uint32	i=0;i<getSize();++i)
			stream<<data[i];
	}

	void	Image::trace()	const{

		std::cout<<"---Image---\n";
		std::cout<<"Size: "<<getSize()<<std::endl;
		std::cout<<"Definition Segment Size: "<<def_size<<std::endl;
		std::cout<<"Object Map Size: "<<map_size<<std::endl;
		std::cout<<"Code Segment Size: "<<code_size<<std::endl;
		std::cout<<"Relocation Segment Size: "<<reloc_size<<std::endl;

		uint32	i=0;

		std::cout<<"===Definition Segment==="<<std::endl;
		for(;i<def_size;++i)
			std::cout<<i<<" "<<data[i]<<std::endl;

		std::cout<<"===Object Map==="<<std::endl;
		for(;i<def_size+map_size;++i)
			std::cout<<i<<" "<<data[i]<<std::endl;

		//	at this point, i is at the first word32 of the first object in the code segment
		std::cout<<"===Code Segment==="<<std::endl;
		uint32	code_start=def_size+map_size;
		for(uint32	j=def_size;j<code_start;++j){	//	read object map: data[data[j]] is the first word32 of an object, data[data[j]+4] is the first atom

			uint32	object_code_size=data[data[j]];
			uint32	object_reference_set_size=data[data[j]+1];
			uint32	object_marker_set_size=data[data[j]+2];
			uint32	object_view_set_size=data[data[j]+3];
			std::cout<<"---object---\n";
			std::cout<<i++<<" code size: "<<object_reference_set_size<<std::endl;
			std::cout<<i++<<" reference set size: "<<object_reference_set_size<<std::endl;
			std::cout<<i++<<" marker set size: "<<object_marker_set_size<<std::endl;
			std::cout<<i++<<" view set size: "<<object_view_set_size<<std::endl;
			
			std::cout<<"---code---\n";
			for(;i<data[j]+4+object_code_size;++i){

				std::cout<<i<<" ";
				((Atom	*)(data+i))->trace();
				std::cout<<std::endl;
			}

			std::cout<<"---reference set---\n";
			for(;i<data[j]+4+object_code_size+object_reference_set_size;++i)
				std::cout<<i<<" "<<data[i]<<std::endl;

			std::cout<<"---marker set---\n";
			for(;i<data[j]+4+object_code_size+object_reference_set_size+object_marker_set_size;++i)
				std::cout<<i<<" "<<data[i]<<std::endl;

			std::cout<<"---view set---\n";
			for(uint32	k=0;k<object_view_set_size;++k){

				uint32	view_code_size=data[i];
				uint32	view_reference_set_size=data[i+1];

				std::cout<<"view["<<k<<"]\n";
				std::cout<<i++<<" code size: "<<view_code_size<<std::endl;
				std::cout<<i++<<" reference set size: "<<view_reference_set_size<<std::endl;

				std::cout<<"---code---\n";
				uint32	l;
				for(l=0;l<view_code_size;++i,++l){

					std::cout<<i<<" ";
					((Atom	*)(data+i))->trace();
					std::cout<<std::endl;
				}

				std::cout<<"---reference set---\n";
				for(l=0;l<view_reference_set_size;++i,++l)
					std::cout<<i<<" "<<data[i]<<std::endl;
			}
		}

		std::cout<<"===Relocation Segment==="<<std::endl;
		uint32	entry_count=data[i];
		std::cout<<i++<<" entries count: "<<entry_count<<std::endl;
		for(uint32	j=0;j<entry_count;++j){

			std::cout<<"entry["<<j<<"]\n";
			uint32	reference_count=data[i++];
			std::cout<<i<<" count: "<<reference_count<<std::endl;
			for(uint32	k=0;k<reference_count;++k){

				std::cout<<i<<" object: "<<data[i++]<<std::endl;
				std::cout<<i<<" view: "<<data[i++]<<std::endl;
				std::cout<<i<<" pointer: "<<data[i++]<<std::endl;
			}
		}
	}
}