export interface AquariumData {
  temperaturaAtual?: number;
  phAtual?: number;
  feederLastTs?: number;

  temperatura?: number;
  ph?: number;

  float?: {
    water_ok?: boolean;
  };

  relay?: {
    heater?: boolean;
    waterfall?: boolean;
  };

  controle?: {
    heater?: {
      mode?: "auto" | "manual";
      turn_on_now?: boolean;
      state?: boolean;
    };
    waterfall?: {
      mode?: "auto" | "manual";
      turn_on_now?: boolean;
      state?: boolean;
    };
    feeder?: {
      feed_now?: boolean;
    };
  };

  status?: {
    last_seen?: number;
    feeder?: {
      busy?: boolean;
      last_ts?: number;
    };
  };

  feeder?: {
    last_ts?: number;
  };
}

export interface AquariumControl {
  feeder?: boolean;
}

export interface AquariumConfig {
  feeder_speed?: number;
  portions_per_day?: number;
  feed_time?: string;
}

export interface HistoricalReading {
  timestamp: number;
  temperatura?: number;
  ph?: number;
}
