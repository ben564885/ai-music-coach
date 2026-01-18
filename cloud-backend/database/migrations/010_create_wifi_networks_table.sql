-- Create WiFi networks table to store saved WiFi credentials for each device
-- Each device can have multiple saved networks (for different locations)
CREATE TABLE IF NOT EXISTS wifi_networks (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id TEXT NOT NULL,
    ssid TEXT NOT NULL,
    password TEXT NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,  -- Mark the currently active network
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    -- Ensure device_id + ssid combination is unique
    UNIQUE(device_id, ssid)
);

-- Index for fast lookups by device_id
CREATE INDEX IF NOT EXISTS idx_wifi_networks_device_id ON wifi_networks(device_id);

-- Index for active networks
CREATE INDEX IF NOT EXISTS idx_wifi_networks_active ON wifi_networks(device_id, is_active) WHERE is_active = TRUE;

-- Enable RLS (Row Level Security)
ALTER TABLE wifi_networks ENABLE ROW LEVEL SECURITY;

-- Policy: Devices can only access their own WiFi networks
CREATE POLICY "Devices can access their own WiFi networks"
    ON wifi_networks
    FOR ALL
    USING (true);  -- Allow all operations for now (devices use X-User-ID header)

-- Add updated_at trigger
CREATE OR REPLACE FUNCTION update_wifi_networks_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER wifi_networks_updated_at
    BEFORE UPDATE ON wifi_networks
    FOR EACH ROW
    EXECUTE FUNCTION update_wifi_networks_updated_at();
