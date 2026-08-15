# Environmental Monitoring Database

## Tables

### sensor_readings
- id (PRIMARY KEY)
- timestamp DATETIME
- temperature FLOAT
- humidity FLOAT
- water_level INT
- gas_level INT
- location_id INT
- risk_probability FLOAT

### emergency_alerts
- alert_id (PRIMARY KEY)
- timestamp DATETIME
- severity ENUM('Low', 'Medium', 'High')
- location VARCHAR(255)
- description TEXT
- status ENUM('Active', 'Resolved')

### location_master
- location_id (PRIMARY KEY)
- latitude DECIMAL(10,8)
- longitude DECIMAL(11,8)
- region VARCHAR(100)
- risk_zone ENUM('Low', 'Medium', 'High')

## Indexing
- Create index on timestamp
- Create index on location_id
- Create composite index on (timestamp, location_id)
