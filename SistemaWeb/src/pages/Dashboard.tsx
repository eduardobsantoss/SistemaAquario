import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { useAuth } from "@/hooks/useAuth";
import { useAquariumData } from "@/hooks/useAquariumData";
import { ref, set } from "firebase/database";
import { database } from "@/lib/firebase";
import { Button } from "@/components/ui/button";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { toast } from "sonner";
import {
  Thermometer,
  Droplets,
  Power,
  Waves,
  Loader2,
  LogOut,
  BarChart3,
  Fish,
  Flame,
  Waves as WavesIcon,
} from "lucide-react";
import { ThemeToggle } from "@/components/ThemeToggle";

const Dashboard = () => {
  const { user, signOut } = useAuth();
  const { data, loading } = useAquariumData();
  const navigate = useNavigate();
  const [controlling, setControlling] = useState(false);

  // ---- ESTADOS DO CONTROLE ----
  const heaterMode = data.controle?.heater?.mode ?? "auto";
  const heaterState =
    data.controle?.heater?.state ?? data.relay?.heater ?? false;

  const waterfallMode = data.controle?.waterfall?.mode ?? "auto";
  const waterfallState =
    data.controle?.waterfall?.state ?? data.relay?.waterfall ?? false;

  // ---- FEEDER ----
  const handleFeedNow = async () => {
    setControlling(true);
    try {
      await set(ref(database, "aquario/controle/feeder/feed_now"), true);
      toast.success("Alimentador acionado!");
    } catch {
      toast.error("Erro ao acionar alimentador");
    } finally {
      setControlling(false);
    }
  };

  // ---- HEATER ----
  const setHeaterMode = async (mode: "auto" | "manual") => {
    try {
      await set(ref(database, "aquario/controle/heater/mode"), mode);
      toast.success(`Aquecedor em modo ${mode}`);
    } catch {
      toast.error("Erro ao mudar modo do aquecedor");
    }
  };

  const toggleHeaterManual = async () => {
    try {
      await set(
        ref(database, "aquario/controle/heater/turn_on_now"),
        !heaterState
      );
    } catch {
      toast.error("Erro ao alternar aquecedor");
    }
  };

  // ---- WATERFALL ----
  const setWaterfallMode = async (mode: "auto" | "manual") => {
    try {
      await set(ref(database, "aquario/controle/waterfall/mode"), mode);
      toast.success(`Cascata em modo ${mode}`);
    } catch {
      toast.error("Erro ao mudar modo da cascata");
    }
  };

  const toggleWaterfallManual = async () => {
    try {
      await set(
        ref(database, "aquario/controle/waterfall/turn_on_now"),
        !waterfallState
      );
    } catch {
      toast.error("Erro ao alternar cascata");
    }
  };

  // ---- AUTH ----
  const handleSignOut = async () => {
    await signOut();
    navigate("/auth");
  };

  // ---- HELPERS ----
  const formatLastSeen = (timestamp?: number) => {
    if (!timestamp) return "Nunca";
    const now = Date.now();
    const diff = now - timestamp;
    const seconds = Math.floor(diff / 1000);
    if (seconds < 60) return `${seconds}s atrás`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes}min atrás`;
    const hours = Math.floor(minutes / 60);
    return `${hours}h atrás`;
  };

  const getTemperatureStatus = (temp?: number) => {
    if (!temp && temp !== 0)
      return { label: "N/A", variant: "secondary" as const };
    if (temp < 24) return { label: "Baixa", variant: "destructive" as const };
    if (temp > 28) return { label: "Alta", variant: "destructive" as const };
    return { label: "OK", variant: "default" as const };
  };

  const getPhStatus = (ph?: number) => {
    if (!ph && ph !== 0) return { label: "N/A", variant: "secondary" as const };
    if (ph < 6.5 || ph > 7.5)
      return { label: "Atenção", variant: "destructive" as const };
    return { label: "Normal", variant: "default" as const };
  };

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-background to-primary/5">
        <Loader2 className="w-8 h-8 animate-spin text-primary" />
      </div>
    );
  }

  const tempStatus = getTemperatureStatus(data.temperaturaAtual);
  const phStatus = getPhStatus(data.phAtual);

  return (
    <div className="min-h-screen bg-gradient-to-br from-background to-primary/5 p-4 md:p-8">
      <div className="max-w-6xl mx-auto space-y-6">
        {/* Header */}
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="p-2 rounded-lg bg-primary/10">
              <Waves className="w-6 h-6 text-primary" />
            </div>
            <div>
              <h1 className="text-2xl font-bold">Dashboard do Aquário</h1>
              <p className="text-sm text-muted-foreground">{user?.email}</p>
            </div>
          </div>
          <div className="flex gap-2">
            <Button
              variant="outline"
              size="sm"
              onClick={() => navigate("/history")}
            >
              <BarChart3 className="w-4 h-4 mr-2" />
              Histórico
            </Button>
            <ThemeToggle />
            <Button variant="outline" size="sm" onClick={handleSignOut}>
              <LogOut className="w-4 h-4 mr-2" />
              Sair
            </Button>
          </div>
        </div>

        {/* Status Connection */}
        <Card>
          <CardContent className="pt-6">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <div
                  className={`w-2 h-2 rounded-full ${
                    data.status?.last_seen
                      ? "bg-green-500 animate-pulse"
                      : "bg-red-500"
                  }`}
                />
                <span className="text-sm font-medium">
                  Sistema {data.status?.last_seen ? "Online" : "Offline"}
                </span>
              </div>
              <span className="text-xs text-muted-foreground">
                Última atualização: {formatLastSeen(data.status?.last_seen)}
              </span>
            </div>
          </CardContent>
        </Card>

        {/* Main Sensors */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          <Card>
            <CardHeader className="pb-3">
              <div className="flex items-center justify-between">
                <CardTitle className="text-sm font-medium">
                  Temperatura
                </CardTitle>
                <Thermometer className="w-4 h-4 text-muted-foreground" />
              </div>
            </CardHeader>
            <CardContent>
              <div className="space-y-2">
                <div className="text-2xl font-bold">
                  {typeof data.temperaturaAtual === "number"
                    ? data.temperaturaAtual.toFixed(1)
                    : "--"}
                  °C
                </div>
                <Badge variant={tempStatus.variant}>{tempStatus.label}</Badge>
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader className="pb-3">
              <div className="flex items-center justify-between">
                <CardTitle className="text-sm font-medium">pH</CardTitle>
                <Droplets className="w-4 h-4 text-muted-foreground" />
              </div>
            </CardHeader>
            <CardContent>
              <div className="space-y-2">
                <div className="text-2xl font-bold">
                  {typeof data.phAtual === "number"
                    ? data.phAtual.toFixed(1)
                    : "--"}
                </div>
                <Badge variant={phStatus.variant}>{phStatus.label}</Badge>
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader className="pb-3">
              <div className="flex items-center justify-between">
                <CardTitle className="text-sm font-medium">
                  Nível da Água
                </CardTitle>
                <Waves className="w-4 h-4 text-muted-foreground" />
              </div>
            </CardHeader>
            <CardContent>
              <div className="space-y-2">
                <div className="text-2xl font-bold">
                  {data.float?.water_ok ? "OK" : "Baixo"}
                </div>
                <Badge
                  variant={data.float?.water_ok ? "default" : "destructive"}
                >
                  {data.float?.water_ok ? "Normal" : "Atenção"}
                </Badge>
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader className="pb-3">
              <div className="flex items-center justify-between">
                <CardTitle className="text-sm font-medium">
                  Dispositivos
                </CardTitle>
                <Power className="w-4 h-4 text-muted-foreground" />
              </div>
            </CardHeader>
            <CardContent>
              <div className="space-y-2">
                <div className="flex items-center justify-between text-sm">
                  <span>Aquecedor</span>
                  <Badge variant={heaterState ? "default" : "secondary"}>
                    {heaterState ? "Ligado" : "Desligado"}
                  </Badge>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span>Cascata</span>
                  <Badge variant={waterfallState ? "default" : "secondary"}>
                    {waterfallState ? "Ligada" : "Desligada"}
                  </Badge>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>

        {/* Controls */}
        <Card>
          <CardHeader>
            <CardTitle>Controles</CardTitle>
            <CardDescription>
              Acione os dispositivos manualmente
            </CardDescription>
          </CardHeader>

          <CardContent className="space-y-4">
            {/* Alimentador */}
            <div className="flex flex-wrap items-center gap-2">
              <Button onClick={handleFeedNow} disabled={controlling}>
                {controlling ? (
                  <Loader2 className="w-4 h-4 mr-2 animate-spin" />
                ) : (
                  <Fish className="w-4 h-4 mr-2" />
                )}
                Alimentar agora
              </Button>
              {data.status?.feeder?.last_ts && (
                <span className="text-sm text-muted-foreground">
                  Última alimentação:{" "}
                  {new Date(data.status.feeder.last_ts).toLocaleString("pt-BR")}
                </span>
              )}
            </div>

            {/* Aquecedor */}
            <div className="grid grid-cols-1 sm:grid-cols-3 gap-2 items-center">
              <div className="font-medium flex items-center gap-2">
                <Flame className="w-4 h-4" /> Aquecedor
              </div>
              <div className="flex gap-2">
                <Button
                  variant={heaterMode === "auto" ? "default" : "outline"}
                  onClick={() => setHeaterMode("auto")}
                >
                  Auto
                </Button>
                <Button
                  variant={heaterMode === "manual" ? "default" : "outline"}
                  onClick={() => setHeaterMode("manual")}
                >
                  Manual
                </Button>
              </div>
              <div className="flex justify-start sm:justify-end">
                <Button
                  variant={heaterState ? "default" : "outline"}
                  disabled={heaterMode !== "manual"}
                  onClick={toggleHeaterManual}
                >
                  {heaterState ? "Desligar" : "Ligar"}
                </Button>
              </div>
            </div>

            {/* Cascata */}
            <div className="grid grid-cols-1 sm:grid-cols-3 gap-2 items-center">
              <div className="font-medium flex items-center gap-2">
                <WavesIcon className="w-4 h-4" /> Cascata
              </div>
              <div className="flex gap-2">
                <Button
                  variant={waterfallMode === "auto" ? "default" : "outline"}
                  onClick={() => setWaterfallMode("auto")}
                >
                  Auto
                </Button>
                <Button
                  variant={waterfallMode === "manual" ? "default" : "outline"}
                  onClick={() => setWaterfallMode("manual")}
                >
                  Manual
                </Button>
              </div>
              <div className="flex justify-start sm:justify-end">
                <Button
                  variant={waterfallState ? "default" : "outline"}
                  disabled={waterfallMode !== "manual"}
                  onClick={toggleWaterfallManual}
                >
                  {waterfallState ? "Desligar" : "Ligar"}
                </Button>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
};

export default Dashboard;
