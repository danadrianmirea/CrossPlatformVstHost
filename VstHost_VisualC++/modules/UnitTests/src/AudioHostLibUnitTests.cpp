#include "gtest/gtest.h"

#include "audiohost.h"
#include "Common.h"
#include "file.h"
#include "enums.h"

namespace AudioHostLibUnitTest
{
    class AudioHostLibTest : public testing::Test
    {
        protected:
            std::unique_ptr<AudioProcessingVstHost> vst_host_lib_;
            
            AudioProcessingVstHost* vst_host_c_api_ = nullptr;

            AudioHostLibTest()          = default;
            virtual ~AudioHostLibTest() = default;
            virtual void SetUp() 
            {
                vst_host_lib_.reset(new AudioProcessingVstHost());  
                CleanUpUtProducts();
            }

            virtual void TearDown() 
            {
                vst_host_lib_.reset();
                if (vst_host_c_api_ != nullptr)
                {
                    CApiDeleteInstance(vst_host_c_api_);
                    vst_host_c_api_ = nullptr;
                }
                CleanUpUtProducts();
            }

            void CleanUpUtProducts()
            {
                if (std::filesystem::exists(OUTPUT_WAVE_PATH))
                {
                    std::remove(OUTPUT_WAVE_PATH.c_str());
                }

                if (std::filesystem::exists(DUMP_JSON_FILE_PATH))
                {
                    std::remove(DUMP_JSON_FILE_PATH.c_str());
                }
            }

            void RemoveDumpedJsonConfig()
            {
                int status = VST_ERROR_STATUS::PATH_NOT_EXISTS;
                if (std::filesystem::exists(DUMP_JSON_FILE_PATH))
                {
                    std::remove(DUMP_JSON_FILE_PATH.c_str());
                    status = VST_ERROR_STATUS::SUCCESS;
                }
                EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
            }

            int LoadJson(std::string plugin_config_path, nlohmann::json* json_config)
            {
                std::ifstream file(plugin_config_path);
                if (!file.is_open())
                {
                    return VST_ERROR_STATUS::OPEN_FILE_ERROR;
                }
                file >> *json_config;
                file.close();
                return VST_ERROR_STATUS::SUCCESS;
            }

            int LoadWave(std::string wave_path, std::vector<float>* data)
            {
                wave::File input_wave_file;
                if (input_wave_file.Open(wave_path, wave::kIn))
                {
                    return VST_ERROR_STATUS::OPEN_FILE_ERROR;
                }

                if (input_wave_file.Read(data))
                {
                
                    return VST_ERROR_STATUS::READ_WRITE_ERROR;
                }
                return VST_ERROR_STATUS::SUCCESS;
            }
    };

    // Positive Tests
    TEST_F(AudioHostLibTest, ProcessWaveFileWithSinglePluginAndDefaultPluginSettings)
    {
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->ProcessWaveFileWithSinglePlugin(INPUT_WAVE_PATH, 
                                                                OUTPUT_WAVE_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        
        std::vector<float> output;
        status = LoadWave(OUTPUT_WAVE_PATH, &output);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);

        std::vector<float> ref;
        status = LoadWave(REF_OUTPUT_DEFAULT_CONFIG, &ref);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        EXPECT_EQ(output, ref);
    }

    TEST_F(AudioHostLibTest, CapiProcessWaveFileWithSinglePluginAndDefaultPluginSettings)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);

        status = CApiProcessWaveFileWithSinglePlugin(vst_host_c_api_,
                                                     INPUT_WAVE_PATH.c_str(),
                                                     OUTPUT_WAVE_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiDeleteInstance(vst_host_c_api_);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        vst_host_c_api_ = nullptr;
        std::vector<float> output;
        status = LoadWave(OUTPUT_WAVE_PATH, &output);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);

        std::vector<float> ref;
        status = LoadWave(REF_OUTPUT_DEFAULT_CONFIG, &ref);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        EXPECT_EQ(output, ref);
    }

    TEST_F(AudioHostLibTest, GetPluginConfig)
    {
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->GetPluginParameters(DUMP_JSON_FILE_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        RemoveDumpedJsonConfig();
    }

    TEST_F(AudioHostLibTest, CApiGetPluginConfig)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiGetPluginParameters(vst_host_c_api_, DUMP_JSON_FILE_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiDeleteInstance(vst_host_c_api_);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        vst_host_c_api_ = nullptr;
        RemoveDumpedJsonConfig();
    }

    TEST_F(AudioHostLibTest, SetPluginConfig)
    {
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->SetPluginParameters(LOAD_JSON_FILE_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->GetPluginParameters(DUMP_JSON_FILE_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
       
        nlohmann::json ref_plugin_config_json;
        status = LoadJson(LOAD_JSON_FILE_PATH, &ref_plugin_config_json);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        nlohmann::json dumped_plugin_config_json;
        status = LoadJson(DUMP_JSON_FILE_PATH, &dumped_plugin_config_json);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        RemoveDumpedJsonConfig();
        for (nlohmann::json::iterator it = ref_plugin_config_json.begin(); it != ref_plugin_config_json.end(); ++it) 
        {
            auto single_param_map = dumped_plugin_config_json.find(it.key());
            EXPECT_EQ(it.value(), *single_param_map);
        }    
    }

    TEST_F(AudioHostLibTest, CApiSetPluginConfig)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiSetPluginParameters(vst_host_c_api_, LOAD_JSON_FILE_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiGetPluginParameters(vst_host_c_api_, DUMP_JSON_FILE_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiDeleteInstance(vst_host_c_api_);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        vst_host_c_api_ = nullptr;

        nlohmann::json ref_plugin_config_json;
        status = LoadJson(LOAD_JSON_FILE_PATH, &ref_plugin_config_json);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        nlohmann::json dumped_plugin_config_json;
        status = LoadJson(DUMP_JSON_FILE_PATH, &dumped_plugin_config_json);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        RemoveDumpedJsonConfig();
        for (nlohmann::json::iterator it = ref_plugin_config_json.begin(); it != ref_plugin_config_json.end(); ++it)
        {
            auto single_param_map = dumped_plugin_config_json.find(it.key());
            EXPECT_EQ(it.value(), *single_param_map);
        }
    }
    
    // Negative Tests
    TEST_F(AudioHostLibTest, CreatePluginInstanceWithEmptyPath)
    {
        int status = vst_host_lib_->CreatePluginInstance("");
        EXPECT_EQ(status, VST_ERROR_STATUS::CREATE_HOSTING_MODULE_ERROR);
    }

    TEST_F(AudioHostLibTest, CApiCreatePluginInstanceWithEmptyPath)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int function_status = CApiCreatePluginInstance(vst_host_c_api_, "");
        EXPECT_EQ(function_status, VST_ERROR_STATUS::CREATE_HOSTING_MODULE_ERROR);
    }

    TEST_F(AudioHostLibTest, SetPluginParametersBeforeCreatingPluginInstance)
    {
        std::string plugin_config;
        int status = vst_host_lib_->SetPluginParameters(plugin_config);
        EXPECT_EQ(status, VST_ERROR_STATUS::NULL_POINTER);
    }

    TEST_F(AudioHostLibTest, CApiSetPluginParametersBeforeCreatingPluginInstance)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        std::string plugin_config;
        int function_status = CApiSetPluginParameters(vst_host_c_api_, plugin_config.c_str());
        EXPECT_EQ(function_status, VST_ERROR_STATUS::NULL_POINTER);
    }

    TEST_F(AudioHostLibTest, SetPluginParametersWithEmptyPluginConfigPath)
    {
        std::string plugin_config;
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->SetPluginParameters(plugin_config);
        EXPECT_EQ(status, VST_ERROR_STATUS::OPEN_FILE_ERROR);
    }

    TEST_F(AudioHostLibTest, CApiSetPluginParametersWithEmptyPluginConfigPath)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        std::string plugin_config;
        int function_status = CApiSetPluginParameters(vst_host_c_api_, plugin_config.c_str());
        EXPECT_EQ(function_status, VST_ERROR_STATUS::OPEN_FILE_ERROR);
    }

    TEST_F(AudioHostLibTest, GetPluginParametersBeforeCreatingPluginInstance)
    {
        std::string plugin_config;
        int status = vst_host_lib_->GetPluginParameters(plugin_config);
        EXPECT_EQ(status, VST_ERROR_STATUS::NULL_POINTER);
    }

    TEST_F(AudioHostLibTest, CApiGetPluginParametersBeforeCreatingPluginInstance)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        std::string plugin_config;
        int status = CApiGetPluginParameters(vst_host_c_api_, plugin_config.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::NULL_POINTER);
    }

    TEST_F(AudioHostLibTest, GetPluginParametersWithEmptyPluginConfigPath)
    {
        std::string plugin_config;
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->GetPluginParameters(plugin_config);
        EXPECT_EQ(status, VST_ERROR_STATUS::OPEN_FILE_ERROR);
    }

    TEST_F(AudioHostLibTest, CApiGetPluginParametersWithEmptyPluginConfigPath)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        std::string plugin_config;
        int function_status = CApiGetPluginParameters(vst_host_c_api_, plugin_config.c_str());
        EXPECT_EQ(function_status, VST_ERROR_STATUS::OPEN_FILE_ERROR);
    }

    TEST_F(AudioHostLibTest, ProcessWaveFileWithSinglePluginBeforeCreatingPluginInstance)
    {
        int status = vst_host_lib_->ProcessWaveFileWithSinglePlugin(INPUT_WAVE_PATH,
                                                                    OUTPUT_WAVE_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::CREATE_PLUGIN_PROVIDER_ERROR);
    }

    TEST_F(AudioHostLibTest, CApiProcessWaveFileWithSinglePluginBeforeCreatingPluginInstance)
    {
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiProcessWaveFileWithSinglePlugin(vst_host_c_api_, 
                                                         INPUT_WAVE_PATH.c_str(),
                                                         OUTPUT_WAVE_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::CREATE_PLUGIN_PROVIDER_ERROR);
    }

    TEST_F(AudioHostLibTest, ProcessWaveFileWithSinglePluginWithEmptyInputOutputWavePath)
    {
        std::string input_wave_path;
        std::string output_wave_path;
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->ProcessWaveFileWithSinglePlugin(input_wave_path, output_wave_path);
        EXPECT_EQ(status, VST_ERROR_STATUS::PATH_NOT_EXISTS);
    }

    TEST_F(AudioHostLibTest, CApiProcessWaveFileWithSinglePluginWithEmptyInputOutputWavePath)
    {
        std::string input_wave_path;
        std::string output_wave_path;
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiProcessWaveFileWithSinglePlugin(vst_host_c_api_,
                                                     input_wave_path.c_str(),
                                                     output_wave_path.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::PATH_NOT_EXISTS);
    }

    TEST_F(AudioHostLibTest, ProcessWaveFileWithSinglePluginWithEmptyOutputWavePath)
    {
        std::string output_wave_path;
        int status = vst_host_lib_->CreatePluginInstance(VST_PLUGIN_PATH);
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = vst_host_lib_->ProcessWaveFileWithSinglePlugin(INPUT_WAVE_PATH, 
                                                                output_wave_path);
        EXPECT_EQ(status, VST_ERROR_STATUS::PATH_NOT_EXISTS);
    }

    TEST_F(AudioHostLibTest, CApiProcessWaveFileWithSinglePluginWithEmptyOutputWavePath)
    {
        std::string output_wave_path;
        vst_host_c_api_ = CApiInitialize();
        EXPECT_TRUE(vst_host_c_api_ != nullptr);
        int status = CApiCreatePluginInstance(vst_host_c_api_, VST_PLUGIN_PATH.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::SUCCESS);
        status = CApiProcessWaveFileWithSinglePlugin(vst_host_c_api_,
                                                     INPUT_WAVE_PATH.c_str(),
                                                     output_wave_path.c_str());
        EXPECT_EQ(status, VST_ERROR_STATUS::PATH_NOT_EXISTS);
    }
}
