#include <filesystem>
#include <fstream>
#include <iostream>

#include "runtime/core/meta/meta_example.h"
#include "_generated/serializer/all_serializer.h"

#include "runtime/core/base/macro.h"

#include "runtime/engine.h"

using namespace Pilot;

void metaExample()
{
    g_runtime_global_context.startSystems("");

    Test1 test1_in;
    test1_in.m_int  = 12;
    test1_in.m_char = 'g';
    int i           = 1;
    test1_in.m_int_vector.emplace_back(&i);

    test1_in.m_float_mat = (Matrix4x4::Identity * 5).toMatrix4x4_();

    Test1 test1_out;
    // test on array
    Test2 test2_in;
    test2_in.m_test_base_array.emplace_back("Test1", &test1_in);
    Test1 Test2_temp;
    test2_in.m_test_base_array.emplace_back("Test1", &Test2_temp);
    test2_in.m_int = 356;

    // serializer & deserializer

    // write Test1_in (object) to Test1_json_in (json)
    auto test1_json_in = PSerializer::write(test1_in);

    std::string test1_context = test1_json_in.dump();

    // read Test1_context (json) to Test1_out (object)
    std::string err;

    auto&& Test1_json = PJson::parse(test1_context, err);
    PSerializer::read(Test1_json, test1_out);
    LOG_INFO(test1_context);

    auto        Test2_json_in = PSerializer::write(test2_in);
    std::string test2_context = Test2_json_in.dump();

    std::fstream out_put("out.txt", std::ios::out);
    out_put << test2_context;
    out_put.flush();
    out_put.close();

    Test2  test2_out;
    auto&& test2_json = PJson::parse(test2_context, err);
    PSerializer::read(test2_json, test2_out);
    LOG_INFO(test2_context.c_str());

    // reflection
    auto                       meta = TypeMetaDef(Test2, &test2_out);
    Reflection::FieldAccessor* fields;
    int                        fields_count = meta.m_meta.getFieldsList(fields);
    for (int i = 0; i < fields_count; ++i)
    {
        auto filed_accesser = fields[i];
        std::cout << filed_accesser.getFieldTypeName() << " " << filed_accesser.getFieldName() << " "
                  << (char*)filed_accesser.get(meta.m_instance) << std::endl;
        if (filed_accesser.isArrayType())
        {
            Reflection::ArrayAccessor array_accesser;
            if (Reflection::TypeMeta::newArrayAccessorFromName(filed_accesser.getFieldTypeName(), array_accesser))
            {
                void* field_instance = filed_accesser.get(meta.m_instance);
                int   count          = array_accesser.getSize(field_instance);
                auto  typeMetaItem   = Reflection::TypeMeta::newMetaFromName(array_accesser.getElementTypeName());
                for (int index = 0; index < count; ++index)
                {
                    std::cout << ":L:" << index << ":R:" << (int*)array_accesser.get(index, field_instance)
                              << std::endl;
                    ;
                }
            }
        }
    }

    g_runtime_global_context.shutdownSystems();
}

int main(int argc, char** argv)
{
    std::filesystem::path executable_path(argv[0]);
    std::filesystem::path config_file_path = executable_path.parent_path() / "MoYuEditor.ini";

    Pilot::PilotEngine* engine = new Pilot::PilotEngine();

    engine->startEngine(config_file_path.generic_string());
    engine->initialize();

    engine->run();

    engine->clear();
    engine->shutdownEngine();

    //metaExample();

	return 0;
}