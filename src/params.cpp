#include "ws/lib.hpp"

void ws::to_json(json& j, const Params& p)
{
    j = json{{"graphType", p.graphType},
             {"shape", p.shape},
             {"report", p.report},
             {"numThreads", p.numThreads},
             {"algType", p.algType},
             {"structSize", p.structSize},
             {"numIterExps", p.numIterExps},
             {"stepSpanningType", p.stepSpanningType},
             {"directed", p.directed},
             {"stealTime", p.stealTime},
             {"allTime", p.allTime},
             {"specialExecution", p.specialExecution}
    };
};

void ws::from_json(const json& j, Params& p)
{
    j.at("graphType").get_to(p.graphType);
    j.at("shape").get_to(p.shape);
    j.at("report").get_to(p.report);
    j.at("numThreads").get_to(p.numThreads);
    j.at("algType").get_to(p.algType);
    j.at("structSize").get_to(p.structSize);
    j.at("numIterExps").get_to(p.numIterExps);
    j.at("stepSpanningType").get_to(p.stepSpanningType);
    j.at("directed").get_to(p.directed);
    j.at("stealTime").get_to(p.stealTime);
    j.at("allTime").get_to(p.allTime);
    j.at("specialExecution").get_to(p.specialExecution);
};
