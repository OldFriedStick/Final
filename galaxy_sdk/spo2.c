#include "spo2.h"
#include "data_struct_def.h"

//采样率一律按50hz
RingArray red_raw_data; //红光原始数据，缓存大小51
RingArray red_base_cache;     //红光基线数据，缓存大小同red_cache_data
RingArray red_mid_data; //红光去基线后数据，缓存大小11
RingArray red_data_cache;//红光预处理后的缓存数据，缓存大小256，5秒

RingArray ir_raw_data; //红外原始数据，缓存大小51
RingArray ir_base_cache;     //红外基线数据，缓存大小同red_cache_data
RingArray ir_mid_data; //红外去基线后数据，缓存大小11
RingArray ir_data_cache;//红外预处理后的缓存数据，缓存大小256，5秒


/**
 * @brief 初始化spo2
 * 
 * @return int 0 if success
 */
int spo2_init(){
    int ret = 0;
    ret += alloc_RingArray(&red_raw_data, 51);
    ret += alloc_RingArray(&red_mid_data, 11);
    ret += alloc_RingArray(&red_data_cache, 256);
    ret += alloc_RingArray(&red_base_cache, 256);

    ret += alloc_RingArray(&ir_raw_data, 51);
    ret += alloc_RingArray(&ir_mid_data, 11);
    ret += alloc_RingArray(&ir_data_cache, 256);
    ret += alloc_RingArray(&ir_base_cache, 256);

    if(ret != 0){
        spo2_exit();
    }

    return ret;
}

void spo2_exit(){
    free_RingArray(&red_raw_data);
    free_RingArray(&red_base_cache);
    free_RingArray(&red_mid_data);
    free_RingArray(&red_data_cache);

    free_RingArray(&ir_raw_data);
    free_RingArray(&ir_base_cache);
    free_RingArray(&ir_mid_data);
    free_RingArray(&ir_data_cache);
}

void spo2_reset(){
    reset_RingArray(&red_raw_data);
    reset_RingArray(&red_base_cache);
    reset_RingArray(&red_mid_data);
    reset_RingArray(&red_data_cache);

    reset_RingArray(&ir_raw_data);
    reset_RingArray(&ir_base_cache);
    reset_RingArray(&ir_mid_data);
    reset_RingArray(&ir_data_cache);
}
/**
 * @brief 预处理ppg数据，ppg采样率默认为50，数据格式为RED_IR
 * 
 * @param  
 * @return 错误的数据个数
 */
int preprocess_data(SampleData *sample)
{
    int i = 0, tmp = 0;
    int red_base = 0, ir_base = 0;
    
    uint16_t cnt = sample->cnt;
    uint8_t label = sample->label;
    int wrong_cnt = 0;
    
    for (i = 0; i < cnt ; i += 2)
    {   
        //需要判断数据是否错误
        ra_insert(&red_raw_data, sample->data[i]);
        if(label != sample->labels[i]) wrong_cnt++;
        ra_insert(&ir_raw_data, sample->data[i+1]);
        if(label != sample->labels[i + 1]) wrong_cnt++;
        
        //去基线
        if (red_raw_data.isfull)
        {
            //计算基线值
            red_base = red_raw_data.total / red_raw_data.size;
            ir_base = ir_raw_data.total / ir_raw_data.size;

            //去基线值
            tmp = red_raw_data.data[Mod_Add(red_raw_data.tail, red_raw_data.size /2 , red_raw_data.size)] - red_base;
            ra_insert(&red_mid_data, tmp);
            tmp = ir_raw_data.data[Mod_Add(ir_raw_data.tail, ir_raw_data.size /2 , ir_raw_data.size)] - ir_base;
            ra_insert(&ir_mid_data, tmp);
           
            if(red_mid_data.isfull){
                //去噪声
                tmp = red_mid_data.total / red_mid_data.size;
                ra_insert(&red_data_cache, tmp);
                tmp = ir_mid_data.total / ir_mid_data.size;
                ra_insert(&ir_data_cache, tmp);
                
                //保存基线数据
                ra_insert(&red_base_cache, red_base);
                ra_insert(&ir_base_cache, ir_base);

            }
        }
    }
    return wrong_cnt;
}

/**
 * @brief 找一段数据中的波峰波谷并保存其值
 * 
 * @param data 数据数组，从tail为数据开始点
 * @param peak 保存波峰值，从0开始
 * @param valley 保存波谷，从0开始
 */
void find_store_peak_valley_value(RingArray *data, RingArray *peak, RingArray *valley){
    RingArray tmp;
    int max = 0;
    int min = 0;
    int i = 0, value = 0;
    alloc_RingArray(&tmp, data->size);
    //先重置peak和valley数组
    reset_RingArray(peak);
    reset_RingArray(valley);

    //一阶差分
    cycle_diff(data, &tmp);
    sign(&tmp, &tmp);

    //处理一阶差分中的0值点，直接设为跟前一个值相同
    for(i = 1; i < data->size; i ++){
        if(tmp.data[i] == 0) tmp.data[i] = tmp.data[i - 1]; 
    }
    
    //二阶差分
    cycle_diff(&tmp, &tmp);

    //统计波峰波谷, 
    //因为差分是后一个减去前一个，所以每次差分都会少一个数据，
    //二阶差分后会少两个数据
    for(i = 0; i < data->size - 2; i++){
        switch (tmp.data[i])
        {
        case -2:    //波峰
            value = data->data[Mod_Add(data->tail, i, data->size)];
            if(value <= -10){
                //除去小于0的波峰和相邻的谷
                if(valley->tail > 0){
                    ra_del(valley);
                }
            }else{
                    ra_insert(peak, value);
            }
            break;
        case 2:     //波谷
            //除去大于0的波谷和相邻的峰
            value = data->data[Mod_Add(data->tail, i, data->size)];
            if(value >= 10){
                if(peak->tail > 0){
                    ra_del(peak);
                }
            }else{
                ra_insert(valley, value);
            }
            break;
        default:
            break;
        }
    }

    //找到最大最小值作为一组波峰波谷，保证始终有一组峰谷
    min = max = data->data[0];
    for(i = 0; i < data->size; i++){
        if(min > data->data[i]) min = data->data[i];
        if(max < data->data[i]) max = data->data[i];
    }

    ra_insert(peak, max);
    ra_insert(valley, min);

    free_RingArray(&tmp);
}

static int preR = 0;       //上一个R值
int spo2_process(PpgData *ppg, int *spo2){
    int ret = 0;
    RingArray red_peak, red_valley, ir_peak, ir_valley;
    int red_AC = 0, red_DC = 0;
    int ir_AC = 0, ir_DC = 0;
    int R = 0;
    if(red_data_cache.isfull){
        //初始化波峰波谷数组
        //每秒不超过5个，5秒钟数据，最多不超过25个
        alloc_RingArray(&red_peak, 32);
        alloc_RingArray(&red_valley, 32);
        alloc_RingArray(&ir_peak, 32);
        alloc_RingArray(&ir_valley, 32);

        //找波峰波谷
        find_store_peak_valley_value(&red_data_cache, &red_peak, &red_valley);
        find_store_peak_valley_value(&ir_data_cache, &ir_peak, &ir_valley);

        //计算SPO2
        if(red_peak.tail && red_valley.tail && ir_peak.tail && ir_valley.tail){
            red_AC = red_peak.total / red_peak.tail - red_valley.total / red_valley.tail;
            red_DC = red_base_cache.total / red_base_cache.size + red_valley.total / red_valley.tail;
            ir_AC  = ir_peak.total / ir_peak.tail - ir_valley.total / ir_valley.tail;
            ir_DC = ir_base_cache.total / ir_base_cache.size + ir_valley.total / ir_valley.tail;
            
            //R值应该是小数，为了不使用浮点数，所以先放大1000倍
            R = (int64_t)1000 * (red_AC * ir_DC) / (ir_AC * red_DC);

            if(R > 1000){
                R = 1000000 / R;
            }
            
            //当采集到错误的数据时，R值不正常
            if(R <= 0 || R > 1000){
                ret = 1;
                goto error;
            }
            if(preR != 0){
                R = preR  / 2 + R / 2;
            }
            preR = R;
            if(R > 600){
                *spo2 = ((R - 600) * (-50) + 91611) / 1000;
            }else{
                *spo2 = (-8236 * R / 100000 * R  + 46451 * R / 1000 + 93390) / 1000;
            }
        }else{
            ret = 1;
        }
error:
        //释放内存
        free_RingArray(&red_peak);
        free_RingArray(&red_valley);
        free_RingArray(&ir_peak);
        free_RingArray(&ir_valley);
    }
    return ret;
}

