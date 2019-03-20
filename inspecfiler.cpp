// IntroFiler.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "pch.h"
#include <cstdio>
#include <cstdlib>
#include <string>


#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "Array.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;


enum class ExprValueType {
	eInvalid,

	eMetadata,
	eDataBlock,

	eData,

	eU8,
	eS8,
	eU16,
	eS16,
	eU32,
	eS32,
	eU64,
	eS64,
};
enum ErrorCode {
	eOk = 0,
	eIndexOutOfBounds,
	eWrongType,
};

struct MetaData;
struct DataBlock;

struct ExpressionValue {
	ExprValueType type = ExprValueType::eInvalid;
	union {
		MetaData* metadata;
		DataBlock* datablock;
		struct {
			u8* ptr;
			u64 size;
		} data;
		u8 value_u8;
		s8 value_s8;
		u16 value_u16;
		s16 value_s16;
		u32 value_u32;
		s32 value_s32;
		u64 value_u64;
		s64 value_s64;
	};

	ErrorCode dereference(u64 index, u8* ptr) {
		if (type == ExprValueType::eData) {
			if (index > data.size) {
				return ErrorCode::eIndexOutOfBounds;
			}
			*ptr = data.ptr[index];
			return ErrorCode::eOk;
		}
		return ErrorCode::eWrongType;
	}

};

enum class ExprSelectorType {
	eInvalid,
	eStaticValue, // -> value

	eMetadata, // index -> node
	eDataBlock, // -> node

	eNode, // node, data -> node

	ePrevNode, // -> node
	eNextNode, // -> node
	eParentNode, // -> node

	eSize, // node -> u64
	eOffset, // node -> u64

	eData, // node -> data
	eDereference, // data, index -> u8
	eValue, // node -> type

	eOperator, // type, value, value -> value
};
enum class OperatorType {
	eAdd,
	eSub,
	eMul,
	eDiv,
	eMod,
	eEq,
};

// value[u8](12) -> 12
// metadata[0].parent[this.parent['section']['']] = 'www'
// type coersion: signed -> unsigned, smaller -> larger


struct ExpressionSelector {
	ExprSelectorType type;
	ExpressionValue static_value;
	ExpressionSelector *arg1, *arg2;
	ExprValueType expr_type;

	ErrorCode evaluate(ExpressionValue* result) {
		switch (type) {
		default:
			return ErrorCode::eWrongType;

		case ExprSelectorType::eStaticValue:
			*result = static_value;
			return ErrorCode::eOk;

		case ExprSelectorType::eMetadata:
		case ExprSelectorType::eDataBlock:
		case ExprSelectorType::eNode:

		case ExprSelectorType::ePrevNode:
		case ExprSelectorType::eNextNode:
		case ExprSelectorType::eParentNode:

		case ExprSelectorType::eSize:
		case ExprSelectorType::eOffset:

		case ExprSelectorType::eData:
		case ExprSelectorType::eDereference:

		case ExprSelectorType::eValue:

		case ExprSelectorType::eOperator:
			return ErrorCode::eWrongType;
		}
	}
	ExprValueType get_type() {
		switch (type) {
		default:
			return ExprValueType::eInvalid;

		case ExprSelectorType::eStaticValue:
			return static_value.type;

		case ExprSelectorType::eMetadata:
			return ExprValueType::eMetadata;
		case ExprSelectorType::eDataBlock:
			return ExprValueType::eDataBlock;
		case ExprSelectorType::eNode:
			if (arg1) return arg1->get_type();
			else return ExprValueType::eInvalid;

		case ExprSelectorType::ePrevNode:
		case ExprSelectorType::eNextNode:
		case ExprSelectorType::eParentNode:
			if (arg1) return arg1->get_type();
			else return ExprValueType::eDataBlock;

		case ExprSelectorType::eSize:
		case ExprSelectorType::eOffset:
			return ExprValueType::eU64;

		case ExprSelectorType::eData:
			return ExprValueType::eData;
		case ExprSelectorType::eDereference:
			return ExprValueType::eU8;

		case ExprSelectorType::eValue:
			return expr_type;

		case ExprSelectorType::eOperator:
			return expr_type;
		}
	}

};


struct GeneralExpression {
	ExpressionValue value;
	ExpressionSelector selector;
};


/*

DataMatcher
	Type
		Static-Data
		Generic-Data
		String
			null-terminating -> bool
			size -> General Expression
			type -> ASCII, UTF-8, ...
	offset		-> General Expression
	size		-> General Expression
	optional	-> bool


	data/size
	uvalue
	svalue

	Expression-Access
		data -> u8*
		data[i] -> u8
		value_u8 -> u8
		value_s8 -> s8
		value_u16 -> u16
		value_s16 -> s16
		value_u32 -> u32
		value_s32 -> s32
		value_u64 -> u64
		value_s64 -> s64
		size -> u64
		offset -> u64




Hidden-Metadata
%

Visible-Metadata
$

Data


*/


struct DataSource {
	unsigned char* data;
	size_t size;
};

DataSource* load_file(const char* file) {

	DataSource* datasource = nullptr;
	FILE *f = fopen(file, "rb");

	if (f) {
		datasource = new DataSource();

		fseek(f, 0, SEEK_END);
		datasource->size = ftell(f);
		fseek(f, 0, SEEK_SET);

		datasource->data = (unsigned char*)malloc(datasource->size);
		fread(datasource->data, datasource->size, 1, f);
		fclose(f);
	}
	return datasource;
}

struct Interpretation {
	u32 id;
	//string name
	//how to convert from and to
	bool dynamic_size;
};

enum class DataBlockType {
	eInvalid,
	eStatic,
	eValue,
};
struct FileFormat;



struct DataBlock {
	u32 id;
	//string selector
	DataBlockType type;

	Endianess endianess;
	u64 offset = 0;
	u64 size = 0;

	//data - interpretation

	bool optional = false;

	u8 *static_data = nullptr;

	FileFormat* root_format;
	u32 parent;
	DynArray<u32> child_blocks;

	DataBlock(FileFormat* format) : 
		id(0), type(DataBlockType::eInvalid), 
		endianess(Endianess::eUndefined),
		offset(0), size(0), optional(false), 
		static_data(nullptr), 
		root_format(format), parent(0), child_blocks() {}

	void init_static_check(u8 *ptr, u64 offset, u64 size, bool optional = false) {
		this->type = DataBlockType::eStatic;
		this->offset = offset;
		this->size = size;
		this->optional = optional;
		this->static_data = ptr;
	}
	void init_value(u64 offset, u64 size, bool optional = false) {
		this->type = DataBlockType::eValue;
		this->offset = offset;
		this->size = size;
		this->optional = optional;
	}
	void add_child_block(DataBlock* child_block) {
		child_blocks.push_back(child_block->id);
		child_block->parent = id;
	}
};

struct MetaData {
	//string selector
};

u8 elf_header[4] = { 0x7F, 0x45, 0x4C, 0x46 };
u8 elf_1_byte = 0x01;
u8 elf_2_byte = 0x02;


struct FileFormat {
	IdArray<DataBlock> data_blocks;
	u32 root_block;

	DataBlock* create_block() {
		return &data_blocks[data_blocks.insert(DataBlock(this))];
	}
};


FileFormat* setup_elf_datadefinition() {
	FileFormat* format = new FileFormat();
	DataBlock* block = format->create_block();
	block->init_static_check(elf_header, 0, 4);
	format->root_block = block->id;

	{
		DataBlock* sub_def = format->create_block();
		sub_def->init_static_check(&elf_1_byte, 4, 1, true);
		format->data_blocks[format->root_block].add_child_block(sub_def);

		sub_def = format->create_block();
		sub_def->init_static_check(&elf_1_byte, 5, 1, true);
		format->data_blocks[format->root_block].add_child_block(sub_def);

		sub_def = format->create_block();
		sub_def->init_static_check(&elf_2_byte, 5, 1, true);
		format->data_blocks[format->root_block].add_child_block(sub_def);

		sub_def = format->create_block();
		sub_def->init_static_check(&elf_1_byte, 6, 1);
		format->data_blocks[format->root_block].add_child_block(sub_def);

		//entry
		sub_def = format->create_block();
		sub_def->init_value(0x18, 4);
		format->data_blocks[format->root_block].add_child_block(sub_def);

		//flags
		sub_def = format->create_block();
		sub_def->init_value(0x1C, 4);
		format->data_blocks[format->root_block].add_child_block(sub_def);

		//section header start
		sub_def = format->create_block();
		sub_def->init_value(0x20, 4);
		format->data_blocks[format->root_block].add_child_block(sub_def);
	}
	{
		DataBlock* sub_def = format->create_block();

		sub_def->init_static_check(&elf_2_byte, 4, 1, true);

		format->data_blocks[format->root_block].add_child_block(sub_def);
	}

	return format;
}

bool parse_source_with_definition (FileFormat* format, u32 data_id, DataSource* data) {
	bool matched = true;
	DataBlock* def = &format->data_blocks[data_id];
	switch (def->type) {
	case DataBlockType::eStatic: {
		u64 off = def->offset;
		for (u64 i = 0; i < def->size; i++) {
			if (data->data[off + i] != def->static_data[i]) {
				printf("Static Data Missmatch 0x%" PRIx8 " != 0x%" PRIx8 "\n", data->data[off + i], def->static_data[i]);
			}
		}
		printf("Static Data Matched\n");
	}break;
	case DataBlockType::eValue: {
		u64 off = def->offset;
		printf("Value 0x");
		for (u64 i = 0; i < def->size; i++) {
			printf("%02" PRIx8, data->data[off + i]);
		}
		printf("\n");
	}break;
	}
	if (matched) {
		for (u32 child_id : def->child_blocks) {
			parse_source_with_definition(format, child_id, data);
		}
	}
	else {
		if (def->optional) {

		}
		else {

		}
	}
	return true;
}

int main() {
	const char* filename = "fibrec.elf";
	DataSource* source = load_file(filename);
	if (!source) {
		printf("Failed to load file %s\n", filename);
	}

	printf("Loaded file %s of size: %zu Bytes\n", filename, source->size);
	printf("%" PRIu8 "\n", source->data[4]);
	FileFormat* format = setup_elf_datadefinition();
	parse_source_with_definition(format, format->root_block, source);

}
