-- Add status column to analyses table for polling support
-- Status can be: 'pending', 'complete', 'error'

ALTER TABLE analyses 
ADD COLUMN IF NOT EXISTS status TEXT DEFAULT 'complete';

-- Update existing records to have 'complete' status
UPDATE analyses SET status = 'complete' WHERE status IS NULL;
