#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/config-store-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace ns3;

void ReceivePacket (Ptr<Socket> socket);
static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,uint32_t pktCount, Time pktInterval );
void test_rts(uint32_t qtd_nos, uint32_t area, Boolean rts, FILE **file_razao, FILE **file_through, int matriz_posicao_nos[][2]);

int main(int argc, char **argv){

  srand(time(NULL));
  int i,k;

  FILE  *razao, *through;
  razao = fopen("Razao", "wt");
  through = fopen("Throughput", "wt");


  int area = 750;
  int qtd_nos = 10;
  matriz_posicao_nos[qtd_nos][2];

  for(i=0;i<qtd_nos;i++){
      matriz_posicao_nos[i][0] = rand()%area;
      matriz_posicao_nos[i][1] = rand()%area;
  }
  // Teste com RTS/CTS
  for(k=0;k<qtd_nos;k++){
    test_rts(qtd_nos,area,TRUE, &razao, &through,matriz_posicao_nos);
  }
  // Teste sem RTS/CTS
  for(k=0;k<qtd_nos;k++){
    test_rts(qtd_nos,area,FALSE, &razao, &through,matriz_posicao_nos);
  }
}

void test_rts(uint32_t qtd_nos, uint32_t area, Boolean rts, FILE **file_razao, FILE **file_through, int matriz_posicao_nos[][2]){
  int i,j;
  int qtd_fluxos = qtd_nos/2;

  if(rts){
     Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(1400));
  }else
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(2500));

   NodeContainer nos;
   nos.Create(qtd_nos);

   NetDeviceContainer dispositivos;
   MobilityHelper mobility;

   float distancia_nos[qtd_nos][qts_nos];

  //  Calculo da distancia entre cada no
   for(i = 0; i < qtd_nos; i++)
       for(j = 0; j < qtd_nos; j++)
           distancia_nos[i][j] = sqrt((matriz_posicao_nos[i][0] - matriz_posicao_nos[j][0]) * (matriz_posicao_nos[i][0] - matriz_posicao_nos[j][0]) + (matriz_posicao_nos[i][1] - matriz_posicao_nos[j][1]) * (matriz_posicao_nos[i][1] - matriz_posicao_nos[j][1]));


  Ptr<ListPositionAllocator> Alocacao_inicial = CreateObject<ListPositionAllocator>();
  //Fazendo as distribuições dos nós na área e utilizando o modelo
  for(uint32_t i = 0; i < qtd_nos; i++)
      Alocacao_inicial->Add (Vector(i,matriz_posicao_nos[i][0],matriz_posicao_nos[i][1]));

  mobility.SetPositionAllocator (Alocacao_inicial);
  //Definindo que não mudará as posições dos nós, até que redefina explicitamente
   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

   //Instalando as configurações nos nós
   mobility.Install (nodes);
   NqosWifiMacHelper wifi_mac = NqosWifiMacHelper::Default ();
   wifi_mac.SetType ("ns3::AdhocWifiMac");

   //Modelo da Camada Física
   YansWifiPhyHelper wifi_phy = YansWifiPhyHelper::Default ();

   //Canal da camada física
   YansWifiChannelHelper canal_wifi = YansWifiChannelHelper::Default ();

  //  Configuracoes definidas pelo professor:
   canal_wifi.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   canal_wifi.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                   "Exponent", DoubleValue(2.7),
                                   "ReferenceLoss", DoubleValue(1),
                                   "ReferenceDistance", DoubleValue(150));

    wifi_phy.Set ("TxPowerStart", DoubleValue(22.0));
    wifi_phy.Set ("TxPowerEnd", DoubleValue(22.0));
    wifi_phy.Set ("TxPowerLevels", UintegerValue(1));

    wifi_phy.Set ("EnergyDetectionThreshold", DoubleValue(-96.0));
    wifi_phy.Set ("CcaMode1Threshold", DoubleValue(-99.0));

    //Depois de configurado, estamos criando o canal
   wifi_phy.SetChannel (canal_wifi.Create ());
   WifiHelper wifi = WifiHelper::Default ();
   wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  //  Setando a taxa de transmissao:
   wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
               "DataMode", StringValue("DsssRate11Mbps"),
               "ControlMode", StringValue("DsssRate11Mbps"),
               "NonUnicastMode",StringValue("DsssRate11Mbps"));

   //Instalando as configurações do wifi
    dispositivos = wifi.Install (wifi_phy, wifi_mac, nos);

    //Criando a variável internet
   InternetStackHelper internet;

   //Instalando as configurações na internet
   internet.Install (nos);

   //Estilo de roteamento
   Ipv4GlobalRoutingHelper::PopulateRoutingTables();

   //Definindo os endereços do ipv4 e o (intervalo de endereços possíveis)
   Ipv4AddressHelper address;
   address.SetBase ("10.0.0.0","255.0.0.0");
   //Mantém um vetor de pares, Ptr<Ipv4> e índice, para a interface
   Ipv4InterfaceContainer interfaces;

   //Alocar para um endereço para cada dispositivo
   interfaces = address.Assign (dispositivos);

   //Instalando o FlowMonitorHelper em todos os nós
   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> monitor = flowmon.InstallAll();

   TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

   uint32_t fluxo = 0, sender, receiver;

   //Vetor que servirá para marcar quem já foi escolhido
   uint32_t No[qtd_nos];

   //Definindo o valor inicial
   for(i = 0; i < qtd_nos; i++) No[i] = qtd_nos;

  // //  ------------------------------------------  ESCOLHA DO FLUXOS ------------------------------------------------

   while (fluxo < qtd_fluxos) {
     sender = rand() % qtd_nos;
     while(No[sender] != qtd_nos)
               sender = rand() % qtd_nos;

    No[sender] = sender;
    float distancia = 990;

  //Escolhendo o Receiver que ainda não foi escolhido e que seja o mais próximo do Sender
     for(i = 0; i < qtd_nos; i++){
             if(No[i] == qtd_nos && distancia_nos[sender][i] < distancia){
                 distancia = distancia_nos[sender][i];
                 receiver = i;
             }
    }
     No[receiver] = receiver;

     std::cout << "Sender: " << sender << "\tReceiver: " << receiver << "\tDistancia: " << distancia_nos[sender][receiver] << "\n";

     //  Cria o socket Receiver e recebe um endereço. Vincula o socket ao endereço.
     Ptr<Socket> recvSink = Socket::CreateSocket (nos.Get (receiver), tid);
     InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 2001);
     recvSink->Bind (local);
     recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

     //Cria o socket Sender e a variável remote para armazenar o endereço do Receiver
       Ptr<Socket> source = Socket::CreateSocket (nos.Get (sender), tid);
       InetSocketAddress remote = InetSocketAddress (interfaces.GetAddress (receiver), 2001);

       source->Connect (remote);

       //Agendando a simulação
       Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
           Seconds (1.0), &GenerateTraffic,
           source, 2200, 1, Seconds (1.0));
     fluxo++;
   }

  //  ------------------------------------------  SIMULACAO ------------------------------------------------
  //  Agendando o stop:
   Simulator::Stop (Seconds (31.0));
   Simulator::Run ();

   monitor->CheckForLostPackets ();
   Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
   std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

   double Razao = 0, vazao = 0;
   for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter){

       fprintf(*razao, "%lf\t", 1.0 * iter->second.rxBytes / iter->second.txBytes);
       Razao += 1.0 * iter->second.rxBytes / iter->second.txBytes;
       vazao +=  8.0 * iter->second.rxBytes / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024;
   }

   fprintf(*file_through, "%lf\t", vazao / qtd_fluxos); // Média da vazão
   fprintf(*file_razao, "%lf\t", Razao / qtd_fluxos); // Média da porcentagem de recebimento com sucesso. Intervalo entre 0 e 1

   Simulator::Destroy ();

}

//Se o pacote for recebido com sucesso, essa função será executada
void ReceivePacket (Ptr<Socket> socket){
   NS_LOG_UNCOND ("No: " << socket->GetNode()->GetId() << "\tRecebido com sucesso");
}

//Função responsável por gerar o tráfego
static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                           uint32_t pktCount, Time pktInterval ){
   if (pktCount > 0){
       socket->Send (Create<Packet> (pktSize));
       Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize, pktCount-1, pktInterval);
   }
   else{
      socket->Close ();
   }
}
