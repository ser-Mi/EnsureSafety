#include <Arduino.h>
#include "SelfType.h"
#include <ArduinoJson.h>  //解析Json数据
#include <painlessMesh.h> //Mesh组网

/*
    中间传输采用Json数据，接收信息格式
{
  "type": 0/1/2,      //特种设备0————操作人员1——————指挥人员2//
  "id": 255,        //ESP32 id(0-255)//
  "whether": 0/1,     //是否开锁，0不开锁，1开锁//
  "HR": 90,        //传入心率//
  "HRvalid": 0/1,     //心率是否有效，0无效，1有效//
  "SPO2": 100,       //血氧数值//
  "SPO2valid":0/1,     //血氧数值是否有效,0无效，1有效//
  "light": 0        //光照强度
}
*/

painlessMesh mesh;
HardwareSerial mySerial1(2);

void sendMessage() //发送
{

}

void receivedCallback(uint32_t from, String &msg) //节点ID和消息
{
    StaticJsonDocument<512> doc;                            //声明一个JsonDocument对象
    DeserializationError error = deserializeJson(doc, msg); //反序列化JSON数据
    if (!error)                                             //检查反序列化是否成功
    {
        MyData.type = doc["type"];
        MyData.id = doc["id"];
        MyData.whether = doc["whether"];
        MyData.HR = doc["HR"];
        MyData.HRvalid = doc["HRvalid"];
        MyData.SPO2 = doc["SPO2"];
        MyData.SPO2valid = doc["SPO2valid"];
        MyData.light = doc["light"];
        /*业务处理*/

        char a[14];
        sprintf(a,"n2.val=%d\xff\xff\xff",MyData.whether);
        mySerial1.write(a);
        if(MyData.whether != digitalRead(2))
            digitalWrite(2, MyData.whether); //开锁
    }
    else
    {
        /*故障函数*/
    }
    doc.clear(); //释放Json对象内存
    Serial.printf("Node %u \t  Msg = %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)                 //新设备添加到网络时，此函数都会获得一个回调函数，并且它将打印语句发送到串口监视器。
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()                            //有新设备接入
{
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)               // 可满足painless网格所需的所有计时需求
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void MeshInit()
{
    mesh.setDebugMsgTypes(ERROR | STARTUP);                //用于记录任何错误| STARTUP消息
    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);      //初始化端口
    mesh.onReceive(&receivedCallback);                     //负责将从其他设备接收到的信息在串口监视器中打印出来；
    mesh.onNewConnection(&newConnectionCallback);          //负责通知有没有新设备接入到 ESP-MESH 网络中；
    mesh.onChangedConnections(&changedConnectionCallback); //负责通知 ESP-MESH 网络连接出现变化，比如有设备离线或有新设备加入等；
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);    //负责打印时间同步信息，以确保 ESP-MESH 网络中所有设备的时间是同步的。
}

void GpioInit()
{
    pinMode(2, OUTPUT); //开锁IO
    digitalWrite(2, LOW);
}

void MeshTask(void *por)
{
    for (;;)
    {
        mesh.update();
        vTaskDelay(100);
    }
}

void setup()
{
    Serial.begin(115200);
    mySerial1.begin(9600, SERIAL_8N1, 16, 17);
    GpioInit();
    MeshInit();
    Serial.println("Init");
    xTaskCreatePinnedToCore(MeshTask, "MeshTask", 10000, NULL, 1, NULL, 0);         //核心0用于Mesh组网
}

void loop()
{
    delay(1000);
}
