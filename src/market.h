// Market information and participants
#pragma once

// STL
#include <map>
#include <variant>
#include <vector>

#ifndef DATAFRAME
#define DATAFRAME

typedef std::map <std::string, double> double_df;
typedef std::map <std::string, std::vector <double>> double_vec_df;

#endif

class market_information_class{
    private:
        std::map <std::string, std::variant <unsigned int, double>> parameter;
        double_vec_df prediction;

    public:
        market_information_class(){
            this->parameter["num_interval"] = (unsigned int) 0;
            this->parameter["price_inflex_demand"] = (double) 3000.;

            std::vector <double> vec_double;
            this->prediction["time_length"] = vec_double;
            this->prediction["electricity_price"] = vec_double;
        }

        std::variant <unsigned int, double> get_parameter_values(std::string key){
            return this->parameter[key];
        }

        std::vector <double> get_prediction_values(std::string key){
            return this->prediction[key];
        }

        // Test the program
        void test(){
            // Insert time length (in hours)
            // 12 5-min time intervals + 23 1-hour intervals
            std::vector <double> time_length;
            unsigned int num_short_range = 12;
            unsigned int num_long_range = 23;
            this->parameter["num_interval"] = num_short_range + num_long_range;
            time_length.reserve(num_short_range + num_long_range);
            for(unsigned int short_iter = 0; short_iter < num_short_range; ++ short_iter){
                time_length.push_back(1. / 12.);
            }
            for(unsigned int short_iter = 0; short_iter < num_short_range; ++ short_iter){
                time_length.push_back(1.);
            }
            this->prediction["time_length"] = time_length;

            // Insert electricity prices
            std::vector <double> electricity_price;
            electricity_price.reserve(time_length.size());
            for(unsigned int tick = 0; tick < time_length.size(); ++ tick){
                electricity_price.push_back(10.);
            }
            this->prediction["electricity_price"] = electricity_price;
        }
};

class market_participant_class{
    private:
        std::map <std::string, std::variant <unsigned int, double_df, std::map <std::string, std::variant <double, double_df>>>> parameter;
        double_vec_df prediction;
        std::map <std::string, std::variant <std::vector <double>, double_vec_df, std::map <std::string, double_vec_df>>> schedule, actual;

    public:
        // Constructor function
        market_participant_class(unsigned int participant_type){
            // Participant type: 0 = cer; 1 = rer; 2 = ordinary prosumer; 3 = res prosumer; 4 = local prosumer
            this->parameter["type"] = (unsigned int) participant_type;

            // Premium of different electricity sources
            double_df premium;
            premium["res"] = 0.;
            premium["lem"] = 0.;
            premium["self"] = 0.;
            this->parameter["premium"] = premium;

            // Technical parameters of BESS
            std::map <std::string, std::variant <double, double_df>> bess_par;
            {
                // Capability
                bess_par["energy"] = (double) 0.;
                bess_par["capacity"] = (double) 0.;
                bess_par["efficiency"] = (double) 1.;

                // Initial soc
                double_df initial_soc;
                initial_soc["self"] = 0.;
                initial_soc["lem"] = 0.;
                initial_soc["rer"] = 0.;
                initial_soc["cer"] = 0.;
                bess_par["initial_soc"] = initial_soc;
            }
            this->parameter["bess"] = bess_par;

            // Prediction
            std::vector <double> vec_double;
            this->prediction["default_demand"] = vec_double;
            this->prediction["res_generation"] = vec_double;
            this->prediction["conv_generation"] = vec_double;

            // Schedule and actual operation for optimization
            // Operation of BESS
            std::map <std::string, std::variant <std::vector <double>, double_vec_df, std::map <std::string, double_vec_df>>> operation;

            double_vec_df accounting;
            accounting["self"] = vec_double;
            accounting["lem"] = vec_double;
            accounting["rer"] = vec_double;
            accounting["cer"] = vec_double;

            std::map <std::string, double_vec_df> bess_var;
            {
                bess_var["ch"] = accounting;
                bess_var["dc"] = accounting;
            }
            operation["bess"] = bess_var;

            // Operation of RES
            operation["res_generation"] = accounting;

            // Operation of demand
            operation["default_demand"] = accounting;

            // Operation of non-RES
            operation["conv_generation"] = vec_double;

            this->schedule = operation;
            this->actual = operation;
        }

        std::map <std::string, std::variant <unsigned int, double_df, std::map <std::string, std::variant <double, double_df>>>> get_parameters(){
            return this->parameter;
        }

        double get_prediction_value(std::string key, unsigned int tick){
            return this->prediction[key][tick];
        }

        // Test the program
        void test(unsigned int num_time, market_information_class &information){
            auto participant_type = std::get <unsigned int> (this->parameter["type"]);
            auto time_length = information.get_prediction_values("time_length");

            std::vector <double> default_demand;
            default_demand.reserve(num_time);
            std::vector <double> conv_generation;
            conv_generation.reserve(num_time);
            std::vector <double> res_generation;
            res_generation.reserve(num_time);

            // Insert default demand for prosumers
            if(participant_type >= 2){
                for(unsigned int tick = 0; tick < num_time; ++ tick){
                    double demand_temp = 1.;
                    double res_temp = 0.;
                    double conv_temp = 0.;

                    demand_temp *= time_length[tick];
                    default_demand.push_back(demand_temp);
                    conv_temp *= time_length[tick];
                    conv_generation.push_back(conv_temp);
                    res_temp *= time_length[tick];
                    res_generation.push_back(res_temp);
                }
            }
            // Insert default demand for retailers
            else{
                // Non-RES retailer
                if(participant_type == 0){
                    for(unsigned int tick = 0; tick < num_time; ++ tick){
                        double demand_temp = 0.;
                        double res_temp = 0.;
                        double conv_temp = 1.;

                        demand_temp *= time_length[tick];
                        default_demand.push_back(demand_temp);
                        conv_temp *= time_length[tick];
                        conv_generation.push_back(conv_temp);
                        res_temp *= time_length[tick];
                        res_generation.push_back(res_temp);
                    }
                }
                // RES retailer
                else{
                    for(unsigned int tick = 0; tick < num_time; ++ tick){
                        double demand_temp = 0.;
                        double res_temp = .5;
                        double conv_temp = 0.;

                        demand_temp *= time_length[tick];
                        default_demand.push_back(demand_temp);
                        conv_temp *= time_length[tick];
                        conv_generation.push_back(conv_temp);
                        res_temp *= time_length[tick];
                        res_generation.push_back(res_temp);
                    }
                }
            }

            // Store prediction time series
            this->prediction["default_demand"] = default_demand;
            this->prediction["res_generation"] = res_generation;
            this->prediction["conv_generation"] = conv_generation;
        }
};

class market_class{
    public:
        market_information_class information;
        std::vector <market_participant_class> participants;

        void test(){
            this->information.test();

            market_participant_class consumer(2);
            market_participant_class res(1);
            market_participant_class conv(0);
            std::vector <market_participant_class> participants;
            participants.reserve(3);
            participants.push_back(consumer);
            participants.push_back(res);
            participants.push_back(conv);
            this->participants = participants;
            this->participants[0].test(std::get <unsigned int> (this->information.get_parameter_values("num_interval")), this->information);
            this->participants[1].test(std::get <unsigned int> (this->information.get_parameter_values("num_interval")), this->information);
        }
};
