#include "jsb_sproto.hpp"
#include "scripting/js-bindings/manual/jsb_conversions.hpp"
#include "scripting/js-bindings/manual/jsb_global.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "sproto.h"
#ifdef __cplusplus
}
#endif

struct encode_ud {
	const se::Value *obj;
	//zval *values;
};

struct decode_ud {
	int mainindex;
	const se::Value *obj;
	const se::Value *mapkey;
	bool list;
};
struct SprotoData {
	sproto *sp;
};

se::Object* __jsb_sproto_object = nullptr;
se::Class* __jsb_sproto_class = nullptr;


SE_DECLARE_FINALIZE_FUNC(js_sproto_finalize)

static bool js_sproto_constructor(se::State& s)
{
	/*cocos2d::ui::Widget* cobj = new (std::nothrow) cocos2d::ui::Widget();*/
	//auto sproto=sproto_create("",0);
	auto cobj = new (std::nothrow) SprotoData();
	s.thisObject()->setPrivateData(cobj);
	return true;
}

SE_BIND_CTOR(js_sproto_constructor, __jsb_sproto_class, js_sproto_finalize)

static bool js_sproto_finalize(se::State& s)
{
	//CCLOGINFO("jsbindings: finalizing JS object %p (cocos2d::ui::Widget)", s.nativeThisObject());
    SprotoData  *spd = (SprotoData*)s.nativeThisObject();
	if (spd)
	{
		sproto_release(spd->sp);
		delete spd;
	}
	
	return true;
}
SE_BIND_FINALIZE_FUNC(js_sproto_finalize)

static bool js_newproto(se::State& s)
{
	const auto& args = s.args();
	size_t argc = args.size();
	bool ok = true;
	if (argc == 1)
	{
		cocos2d::Data  content;
		seval_to_Data(args[0],&content);
		auto sp=sproto_create(content.getBytes(), content.getSize());
		auto cobj = (SprotoData*)s.nativeThisObject();
		if (sp&&cobj)
		{
			cobj->sp = sp;
		}
		
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}

SE_BIND_FUNC(js_newproto)

static bool js_newproto_from_file(se::State& s)
{
	const auto& args = s.args();
	size_t argc = args.size();
	bool ok = true;
	if (argc == 1)
	{
		std::string  file;
		seval_to_std_string(args[0], &file);
		int r = 0,len=0;
		char *buffer = nullptr;
		FILE* f = fopen(file.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer =(char*) malloc(len + 1);
		while (r = fread(buffer + r, 1, 1024, f))
		{
		}
		
		auto sp = sproto_create(buffer, len);
		auto cobj = (SprotoData*)s.nativeThisObject();
		if (sp&&cobj)
		{
			cobj->sp = sp;
		}
		else {
			free(buffer);
		}
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}

SE_BIND_FUNC(js_newproto_from_file)

static bool js_deleteproto(se::State& s)
{

	//struct sproto * sp = lua_touserdata(L, 1);
	//if (sp == NULL) {
	//	return luaL_argerror(L, 1, "Need a sproto object");
	//}
	//sproto_release(sp);

	return true;
}
SE_BIND_FUNC(js_deleteproto)

static bool js_dumpproto(se::State& s)
{
	auto cobj = (SprotoData*)s.nativeThisObject();
	if (cobj->sp)
	{
		sproto_dump(cobj->sp);
	}
	else {
		SE_REPORT_ERROR("no proto file loaded");
	}
	

	return true;
}
SE_BIND_FUNC(js_dumpproto)

static bool js_querytype(se::State& s)
{
	auto cobj = (SprotoData*)s.nativeThisObject();
	if (!cobj->sp)
	{
		return false;
	}
	const auto& args = s.args();
	size_t argc = args.size();
	if (argc == 1)
	{
		std::string type;
		seval_to_std_string(args[0],&type);
		auto stype=sproto_type(cobj->sp,type.c_str());
		if (stype)
		{
			se::HandleObject obj(se::Object::createPlainObject());
			obj->setProperty("name", se::Value("sproto_type"));
			
			s.rval().setObject(obj);
		}
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}
SE_BIND_FUNC(js_querytype)


static int decode(const struct sproto_arg *args)
{
	struct decode_ud *self = (struct decode_ud *)args->ud;
	const char *tagname = args->tagname;
	int tagid = args->tagid;
	int type = args->type;
	int index = args->index;
	int mainindex = args->mainindex;
	int length = args->length;
	auto obj = self->obj;
	se::Value data, arr;
	if (index != 0)
	{

		if (!obj->toObject()->getProperty(tagname, &data)) {
			obj->toObject()->setProperty(tagname, data);
			self->list = 1;
			obj = &data;
			if (index < 0)
			{
				return 0;
			}
		}
	}

	struct decode_ud sub = { 0 };
	se::Value narr;
	int r = 0;
	switch (type)
	{
	case SPROTO_TINTEGER:
		if (length == 4)
		{
			data.setUint32(*(uint32_t*)args->value);
		}
		else if (length == 8)
		{
			data.setUlong(*(uint64_t*)args->value);
			//ZVAL_LONG(&data, *(uint64_t*)args->value);
		}
		else {
			SE_REPORT_ERROR("unexpected integer length");
			return 0;
		}
		break;
	case SPROTO_TBOOLEAN:
		data.setBoolean(*(uint32_t*)args->value);
		break;
	case SPROTO_TSTRING:
	{
		std::string val((const char*)args->value, args->length);
		data.setString(val);
	}
		break;
	case SPROTO_TSTRUCT:
		sub.obj = &narr;
		if (args->mainindex >= 0)
		{
			sub.mainindex = args->mainindex;
			r = sproto_decode(args->subtype, args->value, length, decode, &sub);
			if (r < 0)
			{
				return SPROTO_CB_ERROR;
			}
			obj->toObject()->setProperty(sub.mapkey->toString().c_str(), *sub.obj);
		}
		else {
			sub.mainindex = -1;
			data = *sub.obj;
			r = sproto_decode(args->subtype, args->value, length, decode, &sub);
			if (r < 0)
			{
				return SPROTO_CB_ERROR;
			}
			if (r != length)
			{
				return r;
			}
			break;
		}
	default:
		break;
	}

	if (data.getType() != se::Value::Type::Null)
	{
		//zend_string *key = zend_string_init(tagname, strlen(tagname), 0);
		
		if (self->list)
		{
			//php_printf("%d %d %d\n",args->index, zend_array_count(Z_ARR_P(obj)),obj);
			//zend_hash_add(Z_ARR_P(self->arr), key, &data);
			obj->toObject()->setArrayElement(args->index - 1, data);
			//zend_hash_index_add(Z_ARR_P(obj), args->index - 1, &data);
		}
		else {
			obj->toObject()->setProperty(tagname, data);
			//zend_hash_add(Z_ARR_P(self->arr), key, &data);
		}
		if (self->mainindex == tagid)
		{
			self->mapkey = &data;
		}
	}
	else {
		if (self->mainindex == tagid)
		{
			SE_REPORT_ERROR("map key type not support");
			return SPROTO_CB_ERROR;
		}
	}
	return 0;
}

static bool js_decode(se::State& s)
{
	auto cobj = (SprotoData*)s.nativeThisObject();
	if (!cobj->sp)
	{
		return false;
	}
	auto& args = s.args();
	size_t argc = args.size();
	if (argc == 2)
	{
		struct decode_ud self = {0};
		std::string type;
		cocos2d::Data buffer;
		se::Value obj;
		seval_to_std_string(args[0], &type);
		seval_to_Data(args[1], &buffer);
		const auto &table = args[1];
		auto stype = sproto_type(cobj->sp, type.c_str());
		if (stype)
		{
			obj.setObject(se::Object::createPlainObject());
			self.obj = &obj;
			int r=sproto_decode(stype,buffer.getBytes(), buffer.getSize(),decode,&self);

			se::HandleObject hobj(se::Object::createArrayObject(2));
			hobj->setArrayElement(0, obj);
			hobj->setArrayElement(1, se::Value(r));
			//Data_to_seval();
			s.rval().setObject(hobj);
		}
		
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}
SE_BIND_FUNC(js_decode)


static int encode(const struct sproto_arg *args)
{
	struct encode_ud *self = (struct encode_ud*)args->ud;
	const char *tagname = args->tagname;
	int tagid = args->tagid;
	int type = args->type;
	int index = args->index;
	int mainindex = args->mainindex;
	int length = args->length;
	auto obj = self->obj;
	se::Value  data;
	if (!obj->toObject()->getProperty(tagname, &data))
	{
		if (index > 0)
		{
			return SPROTO_CB_NOARRAY;
		}
		return SPROTO_CB_NIL;
	}
	if (index > 0)
	{
		if (data.getType() ==se::Value::Type::Object )
		{
			uint32_t len;
			data.toObject()->getArrayLength(&len);
			if ((uint32_t)index > len)
			{
				return SPROTO_CB_NIL;
			}
			data.toObject()->getArrayElement(index-1,&data);
		}
	}

	struct encode_ud sub;
	int r = 0;
	uint64_t i, vh;

	switch (type)
	{
		case SPROTO_TINTEGER:
			if (data.getType() == se::Value::Type::Number)
			{
				i = data.toUlong();
				vh = i >> 31;
				if (vh == 0 || vh == -1)
				{
					*(uint32_t*)args->value = (uint32_t)i;
					return 4;
				}
				else {
					*(uint64_t*)args->value = (uint64_t)i;
					return 8;
				}
			}
			else {
				SE_REPORT_ERROR("type mismatch, tag:%s, expected int or long", tagname);
				return SPROTO_CB_ERROR;
			}
			break;
		case SPROTO_TBOOLEAN:
			*(uint32_t*)args->value = (uint32_t)data.toBoolean();
			r = 4;
			break;
		case SPROTO_TSTRING:
			if (data.getType() != se::Value::Type::String)
			{
				SE_REPORT_ERROR("type mismatch, tag:%s, expected strng", tagname);
				return SPROTO_CB_ERROR;
			}
			if (data.toString().length() > (uint32_t)length)
			{
				return SPROTO_CB_ERROR;
			}
			memcpy(args->value, data.toString().c_str(), data.toString().size());
			return data.toString().size();
		case SPROTO_TSTRUCT:
			sub.obj = &data;
			r = sproto_encode(args->subtype, args->value, length, encode, &sub);
			if (r < 0)
				return SPROTO_CB_ERROR;
			return r;
		default:
			return 0;
	}
	return r;

}
static bool js_encode(se::State& s)
{
	auto cobj = (SprotoData*)s.nativeThisObject();
	if (!cobj->sp)
	{
		return false;
	}
	auto& args = s.args();
	size_t argc = args.size();
	if (argc == 2)
	{
		struct encode_ud self;
		std::string type;
		seval_to_std_string(args[0], &type);
		const auto &table=args[1];
		auto stype = sproto_type(cobj->sp, type.c_str());
		if (stype)
		{
			int sz = 4096;
			char *buffer =(char*) malloc(sz);
			self.obj = &args[1];
			for (;;)
			{
				int r=sproto_encode(stype, buffer,sz,encode,&self);
				if (r < 0)
				{
					buffer = (char*)realloc(buffer, sz * 2);
					sz *= 2;
				}
				else {
					//php_printf(" len=%d\n",r);
					//v8::String::Utf8Value retval(buffer);
					se::HandleObject dataObj(se::Object::createUint8TypedArray((uint8_t*)buffer, r));
					//jsObj->setProperty("data", se::Value(dataObj));
					//s.rval().toObject()->setProperty("data", se::Value(dataObj));
					s.rval().setObject(dataObj);
					//Data_to_seval();
					return true;
					//free(buffer);
				}
			}
			
			
		}
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}
SE_BIND_FUNC(js_encode)

static bool js_protocol(se::State& s)
{
	return true;
}
SE_BIND_FUNC(js_protocol)

static bool js_loadproto(se::State& s)
{
	return true;
}
SE_BIND_FUNC(js_loadproto)

static bool js_saveproto(se::State& s)
{
	return true;
}
SE_BIND_FUNC(js_saveproto)

static bool js_default(se::State& s)
{
	return true;
}
SE_BIND_FUNC(js_default)


static bool js_pack(se::State& s)
{
	const auto& args = s.args();
	size_t argc = args.size();
	if (argc == 1)
	{
		
		std::string  buffer;
		seval_to_std_string(args[0], &buffer);
		size_t buffer_len= buffer.size();
		size_t maxsz = (buffer_len + 2047) / 2048 * 2 + buffer_len + 2;
		int sz = 4096;
		
		std::string output;
		output.resize(maxsz);

		int bytes = sproto_pack(buffer.c_str(), buffer.size(), &output[0], maxsz);
		cocos2d::Data data;
		data.copy((const uint8_t*)output.data(),output.size());
		Data_to_seval(data,&s.rval());
		//std_string_to_seval(output,&s.rval());
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return false;
}
SE_BIND_FUNC(js_pack)

static bool js_unpack(se::State& s)
{
	const auto& args = s.args();
	size_t argc = args.size();
	if (argc == 1)
	{
		std::string  content;
		cocos2d::Data data;
		seval_to_Data(args[0], &data);
		//size_t sz = content.size();
		std::string output;
		output.resize(1024);
		int r = sproto_unpack(data.getBytes(), data.getSize(), &output[0], output.size());
		if (r > 1024)
		{
			output.resize(r);
			sproto_unpack(data.getBytes(), data.getSize(), &output[0], output.size());
		}
		s.rval().setString(output.c_str());
		return true;
	}
	SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 0);
	return true;
	return true;
}
SE_BIND_FUNC(js_unpack)

bool js_register_Sproto(se::Object* obj)
{
	auto cls = se::Class::create("Sproto", obj, __jsb_sproto_object, _SE(js_sproto_constructor));

	cls->defineFunction("newproto", _SE(js_newproto));
	cls->defineFunction("newproto_from_file", _SE(js_newproto_from_file));
	cls->defineFunction("deleteproto", _SE(js_deleteproto));
	cls->defineFunction("dumpproto", _SE(js_dumpproto));
	cls->defineFunction("querytype", _SE(js_querytype));
	cls->defineFunction("decode", _SE(js_decode));
	cls->defineFunction("encode", _SE(js_encode));
	cls->defineFunction("protocol", _SE(js_protocol));
	cls->defineFunction("loadproto", _SE(js_loadproto));
	cls->defineFunction("saveproto", _SE(js_saveproto));
	cls->defineFunction("default", _SE(js_default));
	cls->defineFunction("pack", _SE(js_pack));
	cls->defineFunction("unpack", _SE(js_unpack));
	cls->defineFinalizeFunction(_SE(js_sproto_finalize));
	cls->install();

	JSBClassType::registerClass<SprotoData>(cls);
	__jsb_sproto_object=cls->getProto();
	__jsb_sproto_class = cls;
	se::ScriptEngine::getInstance()->clearException();
	return true;
}
bool register_all_sproto(se::Object* obj)
{
	se::Value nsVal;
	if (!obj->getProperty("sproto", &nsVal))
	{
		se::HandleObject jsobj(se::Object::createPlainObject());
		nsVal.setObject(jsobj);
		obj->setProperty("sproto", nsVal);
	}
	se::Object* ns = nsVal.toObject();
	js_register_Sproto(ns);
	return true;
}

