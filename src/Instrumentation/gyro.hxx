// gyro.hxx - simple model of a spinning gyro.

#ifndef __INSTRUMENTATION_GYRO_HXX
#define __INSTRUMENTATION_GYRO_HXX 1

/**
 * Simple model of a spinning gyro.
 *
 * The gyro decelerates gradually if no power is available to keep it
 * spinning, and spins up quickly when power becomes available.
 */
class Gyro
{
public:

    /**
     * Constructor.
     */
    Gyro ();


    /**
     * Destructor.
     */
    virtual ~Gyro ();


    /**
     * Update the gyro.
     *
     * @param delta_time_sec The elapsed time since the last update.
     * @param power_norm The power available to drive the gyro, from
     *        0.0 to 1.0.
     */
    virtual void update (double delta_time_sec);


    /**
     * Set the power available to the gyro.
     *
     * @param power_norm The amount of power (vacuum or electrical)
     *        available to keep the gyro spinning, from 0.0 (none) to
     *        1.0 (full power)
     */
    virtual void set_power_norm (double power_norm);


    /**
     * Get the gyro's current spin.
     *
     * @return The spin from 0.0 (not spinning) to 1.0 (full speed).
     */
    virtual double get_spin_norm () const;


    /**
     * Set the gyro's current spin.
     *
     * @spin_norm The spin from 0.0 (not spinning) to 1.0 (full speed).
     */
    virtual void set_spin_norm (double spin_norm);


    /**
     * Test if the gyro is serviceable.
     *
     * @return true if the gyro is serviceable, false otherwise.
     */
    virtual bool is_serviceable () const;


    /**
     * Set the gyro's serviceability.
     *
     * @param serviceable true if the gyro is functional, false otherwise.
     */
    virtual void set_serviceable (bool serviceable);


private:

    bool _serviceable;
    double _power_norm;
    double _spin_norm;

};

#endif // __INSTRUMENTATION_GYRO_HXX
