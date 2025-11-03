import { useState, useEffect } from 'react';
import { ref, onValue, off, query, limitToLast } from 'firebase/database';
import { database } from '@/lib/firebase';
import { AquariumData } from '@/types/aquarium';

type UseAquariumData = {
  data: AquariumData & {
    temperaturaAtual?: number;
    phAtual?: number;
    feederLastTs?: number;
  };
  loading: boolean;
};

export function useAquariumData(): UseAquariumData {
  const [data, setData] = useState<UseAquariumData['data']>({});
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const rootRef = ref(database, 'aquario');
    const unsubRoot = onValue(rootRef, (snap) => {
      const v = snap.val() || {};
      const feederLastTs = v?.feeder?.last_ts ?? v?.status?.feeder?.last_ts;
      setData((prev) => ({ ...prev, ...v, feederLastTs }));
      setLoading(false);
    });

    const tempRef = query(ref(database, 'aquario/temperatura'), limitToLast(1));
    const unsubTemp = onValue(tempRef, (snap) => {
      let ultima: number | undefined = undefined;
      snap.forEach((child) => {
        ultima = Number(child.val());
      });
      setData((prev) => ({ ...prev, temperaturaAtual: ultima }));
    });

    const phRef = query(ref(database, 'aquario/ph'), limitToLast(1));
    const unsubPh = onValue(phRef, (snap) => {
      let ultima: number | undefined = undefined;
      snap.forEach((child) => {
        ultima = Number(child.val());
      });
      setData((prev) => ({ ...prev, phAtual: ultima }));
    });

    return () => {
      off(rootRef, 'value', unsubRoot as any);
      off(tempRef, 'value', unsubTemp as any);
      off(phRef, 'value', unsubPh as any);
    };
  }, []);

  return { data, loading };
}
