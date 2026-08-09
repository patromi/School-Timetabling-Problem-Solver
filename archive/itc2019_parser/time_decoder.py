import numpy as np

class TimeDecoder:
    def __init__(self, nr_days, nr_weeks, slots_per_day):
        self.nr_days = nr_days
        self.nr_weeks = nr_weeks
        self.slots_per_day = slots_per_day
        self.total_slots = nr_days * nr_weeks * slots_per_day

    def decode_time_option(self, days_mask, weeks_mask, start_slot, length):
        """
        Konwertuje specyfikację czasu z XML (days, weeks, start, length) 
        na płaską tablicę wszystkich absolutnych slotów semestru zajmowanych przez tę lekcję.
        """
        active_absolute_slots = []
        
        # Iterate over each week and day, checking if they are active based on the provided masks
        for w_idx, week_bit in enumerate(weeks_mask):
            if week_bit == '1':
                for d_idx, day_bit in enumerate(days_mask):
                    if day_bit == '1':
                        
                        # Calculate the base slot index for this week and day
                        base_slot = (w_idx * self.nr_days * self.slots_per_day) + (d_idx * self.slots_per_day) + start_slot
                    
                        meeting_slots = range(base_slot, base_slot + length)
                        active_absolute_slots.extend(meeting_slots)
                        
        return np.array(active_absolute_slots, dtype=np.int32)