import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { ref, query, orderByKey, limitToLast, onValue, off } from 'firebase/database';
import { database } from '@/lib/firebase';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { ArrowLeft, Loader2 } from 'lucide-react';
import { HistoricalReading } from '@/types/aquarium';

const History = () => {
  const navigate = useNavigate();
  const [data, setData] = useState<HistoricalReading[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const tempRef = query(ref(database, 'aquario/temperatura'), orderByKey(), limitToLast(100));
    const phRef   = query(ref(database, 'aquario/ph'),          orderByKey(), limitToLast(100));

    const combine = (temps?: Record<string, number>, phs?: Record<string, number>) => {
      const readings: Record<number, HistoricalReading> = {};
      if (temps) {
        for (const k of Object.keys(temps)) {
          const ts = Number(k);
          if (!isNaN(ts)) readings[ts] = { ...(readings[ts] || { timestamp: ts }), temperatura: temps[k] };
        }
      }
      if (phs) {
        for (const k of Object.keys(phs)) {
          const ts = Number(k);
          if (!isNaN(ts)) readings[ts] = { ...(readings[ts] || { timestamp: ts }), ph: phs[k] };
        }
      }
      return Object.values(readings).sort((a, b) => a.timestamp - b.timestamp);
    };

    let lastTemps: Record<string, number> | undefined;
    let lastPhs:   Record<string, number> | undefined;

    const unsubTemp = onValue(tempRef, (snap) => {
      lastTemps = snap.exists() ? snap.val() : undefined;
      setData(combine(lastTemps, lastPhs));
      setLoading(false);
    });

    const unsubPh = onValue(phRef, (snap) => {
      lastPhs = snap.exists() ? snap.val() : undefined;
      setData(combine(lastTemps, lastPhs));
      setLoading(false);
    });

    return () => {
      off(tempRef, 'value', unsubTemp as any);
      off(phRef, 'value', unsubPh as any);
    };
  }, []);

  const formatTime = (timestamp: number) =>
    new Date(timestamp).toLocaleTimeString('pt-BR', { hour: '2-digit', minute: '2-digit' });

  const chartData = data.map(r => ({ time: formatTime(r.timestamp), temperatura: r.temperatura, ph: r.ph }));

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gradient-to-br from-background to-primary/5">
        <Loader2 className="w-8 h-8 animate-spin text-primary" />
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-background to-primary/5 p-4 md:p-8">
      <div className="max-w-6xl mx-auto space-y-6">
        <div className="flex items-center gap-4">
          <Button variant="outline" size="sm" onClick={() => navigate('/dashboard')}>
            <ArrowLeft className="w-4 h-4 mr-2" />
            Voltar
          </Button>
          <div>
            <h1 className="text-2xl font-bold">Histórico de Leituras</h1>
            <p className="text-sm text-muted-foreground">Últimas {data.length} leituras registradas (ao vivo)</p>
          </div>
        </div>

        <Card>
          <CardHeader>
            <CardTitle>Temperatura da Água</CardTitle>
            <CardDescription>Variação de temperatura ao longo do tempo</CardDescription>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={300}>
              <LineChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" className="stroke-muted" />
                <XAxis dataKey="time" className="text-xs" tick={{ fill: 'hsl(var(--muted-foreground))' }} />
                <YAxis domain={[20, 30]} className="text-xs" tick={{ fill: 'hsl(var(--muted-foreground))' }} label={{ value: '°C', angle: -90, position: 'insideLeft' }} />
                <Tooltip contentStyle={{ backgroundColor: 'hsl(var(--card))', border: '1px solid hsl(var(--border))', borderRadius: '6px' }} />
                <Legend />
                <Line type="monotone" dataKey="temperatura" stroke="hsl(var(--chart-1))" strokeWidth={2} dot={{ fill: 'hsl(var(--chart-1))' }} name="Temperatura (°C)" />
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle>Nível de pH</CardTitle>
            <CardDescription>Variação do pH ao longo do tempo</CardDescription>
          </CardHeader>
          <CardContent>
            <ResponsiveContainer width="100%" height={300}>
              <LineChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" className="stroke-muted" />
                <XAxis dataKey="time" className="text-xs" tick={{ fill: 'hsl(var(--muted-foreground))' }} />
                <YAxis domain={[6, 8]} className="text-xs" tick={{ fill: 'hsl(var(--muted-foreground))' }} label={{ value: 'pH', angle: -90, position: 'insideLeft' }} />
                <Tooltip contentStyle={{ backgroundColor: 'hsl(var(--card))', border: '1px solid hsl(var(--border))', borderRadius: '6px' }} />
                <Legend />
                <Line type="monotone" dataKey="ph" stroke="hsl(var(--chart-2))" strokeWidth={2} dot={{ fill: 'hsl(var(--chart-2))' }} name="pH" />
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>
      </div>
    </div>
  );
};

export default History;
