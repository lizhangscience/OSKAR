#
#  This file is part of OSKAR.
#
# Copyright (c) 2016, The University of Oxford
# All rights reserved.
#
#  This file is part of the OSKAR package.
#  Contact: oskar at oerc.ox.ac.uk
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. Neither the name of the University of Oxford nor the names of its
#     contributors may be used to endorse or promote products derived from this
#     software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

from __future__ import absolute_import, division
try:
    from . import _simulator_lib
except ImportError:
    _simulator_lib = None
from threading import Thread
from oskar.vis_block import VisBlock
from oskar.vis_header import VisHeader
from oskar.barrier import Barrier


class Simulator(object):
    """This class provides a Python interface to the OSKAR simulator."""

    def __init__(self, precision='double'):
        """Creates a handle to an OSKAR simulator.

        Args:
            precision (str): Either 'double' or 'single' to specify
                the numerical precision of the simulation.
        """
        if _simulator_lib is None:
            raise RuntimeError("OSKAR library not found.")
        self._capsule = _simulator_lib.create(precision)
        self._barrier = None

    def check_init(self):
        """Initialises the simulator if it has not already been done.

        All simulator options and data must have been set appropriately
        before calling this function.
        """
        _simulator_lib.check_init(self._capsule)

    def finalise_block(self, block_index):
        """Finalises a visibility block.

        This method should be called after all prior calls to run_block()
        have completed for a given simulation block.

        Args:
            block_index (int): The simulation block index to finalise.

        Returns:
            block (oskar.VisBlock): A handle to the finalised block.
                This is only valid until the next block is simulated.
        """
        block = VisBlock()
        block._capsule = _simulator_lib.finalise_block(
            self._capsule, block_index)
        return block

    def finalise(self):
        """Finalises the simulator.

        This method should be called after all blocks have been simulated.
        It is not necessary to call this if using the run() method.
        """
        _simulator_lib.finalise(self._capsule)

    def get_coords_only(self):
        """Returns whether the simulator provides baseline coordinates only.

        Returns:
            bool: If set, simulate coordinates only.
        """
        return _simulator_lib.coords_only(self._capsule)

    def get_num_devices(self):
        """Returns the number of compute devices selected.

        Returns:
            int: The number of compute devices selected.
        """
        return _simulator_lib.num_devices(self._capsule)

    def get_num_gpus(self):
        """Returns the number of GPUs selected.

        Returns:
            int: The number of GPUs selected.
        """
        return _simulator_lib.num_gpus(self._capsule)

    def get_num_vis_blocks(self):
        """Returns the number of visibility blocks required for the simulation.

        Returns:
            int: The number of visibility blocks required for the simulation.
        """
        return _simulator_lib.num_vis_blocks(self._capsule)

    def process_block(self, block, block_index):
        """Virtual function to process each visibility block in a worker thread.

        The default implementation simply calls write_block() to write the
        data to any open files. Inherit this class and override this method
        to process the visibilities differently.

        Args:
            block (oskar.VisBlock): A handle to the block to be processed.
            block_index (int):      The index of the visibility block.
        """
        self.write_block(block, block_index)

    def reset_cache(self):
        """Low-level function to reset the simulator's internal memory.
        """
        _simulator_lib.reset_cache(self._capsule)

    def reset_work_unit_index(self):
        """Low-level function to reset the work unit index.

        This must be called after run_block() has returned, for each block.
        """
        _simulator_lib.reset_work_unit_index(self._capsule)

    def run_block(self, block_index, device_id=0):
        """Runs the simulator for one visibility block.

        Multiple compute devices can be used to simulate each block.
        For multi-device simulations, the method should be called multiple
        times using different device IDs from different threads, but with
        the same block index. Device IDs are zero-based.

        This method should be called only after setting all required options.

        Call finalise_block() with the same block index to finalise the block
        after calling this method.

        Args:
            block_index (int): The simulation block index.
            device_id (Optional[int]): The device ID to use for this call.
        """
        _simulator_lib.run_block(self._capsule, block_index, device_id)

    def run(self):
        """Runs the simulator.

        The method process_block() is called for each simulated visibility
        block, where a handle to the block is supplied as an argument.
        Inherit this class and override process_block() to process the
        visibilities differently.
        """
        self.check_init()
        self.reset_work_unit_index()
        num_threads = self.num_devices + 1
        self._barrier = Barrier(num_threads)
        threads = []
        for i in range(num_threads):
            threads.append(Thread(target=self._run_blocks, args=[i]))
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        return self.finalise()

    def set_coords_only(self, value):
        """Sets whether the simulator provides baseline coordinates only.

        Args:
            value (bool): If set, simulate coordinates only.
        """
        _simulator_lib.set_coords_only(self._capsule, value)

    def set_gpus(self, device_ids):
        """Sets the GPU device IDs to use.

        Args:
            device_ids (int, array-like or None):
                A list of the GPU IDs to use, or -1 to use all.
                If None, then no GPUs will be used.
        """
        _simulator_lib.set_gpus(self._capsule, device_ids)

    def set_horizon_clip(self, value):
        """Sets whether horizon clipping is performed.

        Args:
            value (bool): If set, apply horizon clipping.
        """
        _simulator_lib.set_horizon_clip(self._capsule, value)

    def set_max_times_per_block(self, value):
        """Sets the maximum number of times in a visibility block.

        Args:
            value (int): Number of time samples per block.
        """
        _simulator_lib.set_max_times_per_block(self._capsule, value)

    def set_num_devices(self, value):
        """Sets the number of compute devices to use.

        A compute device may be either a local CPU core, or a GPU.
        To use only a single CPU core for simulation, and no GPUs, call:

        set_gpus(None)
        set_num_devices(1)

        Args:
            value (int): Number of compute devices to use.
        """
        _simulator_lib.set_num_devices(self._capsule, value)

    def set_observation_frequency(self, start_frequency_hz,
                                  inc_hz=0.0, num_channels=1):
        """Sets observation start frequency, increment, and number of channels.

        Args:
            start_frequency_hz (float): Frequency of the first channel, in Hz.
            inc_hz (Optional[float]): Frequency increment, in Hz.
            num_channels (Optional[int]): Number of frequency channels.
        """
        _simulator_lib.set_observation_frequency(
            self._capsule, start_frequency_hz, inc_hz, num_channels)

    def set_observation_time(self, start_time_mjd_utc, length_sec,
                             num_time_steps):
        """Sets observation start time, length, and number of samples.

        Args:
            start_time_mjd_utc (float): Observation start time, as MJD(UTC).
            length_sec (float): Observation length in seconds.
            num_time_steps (int): Number of time steps to simulate.
        """
        _simulator_lib.set_observation_time(
            self._capsule, start_time_mjd_utc, length_sec, num_time_steps)

    def set_output_measurement_set(self, filename):
        """Sets the name of the output CASA Measurement Set.

        Args:
            filename (str): Output filename.
        """
        _simulator_lib.set_output_measurement_set(self._capsule, filename)

    def set_output_vis_file(self, filename):
        """Sets the name of the output OSKAR visibility file.

        Args:
            filename (str): Output filename.
        """
        _simulator_lib.set_output_vis_file(self._capsule, filename)

    def set_settings_path(self, filename):
        """Sets the path to the input settings file or script.

        This is used only to store the file in the output visibility data.

        Args:
            filename (str): Filename.
        """
        _simulator_lib.set_settings_path(self._capsule, filename)

    def set_sky_model(self, sky_model, max_sources_per_chunk=16384):
        """Sets the sky model used for the simulation.

        Args:
            sky_model (oskar.Sky): Sky model object.
            max_sources_per_chunk (int): Maximum number of sources per chunk.
        """
        _simulator_lib.set_sky_model(
            self._capsule, sky_model._capsule, max_sources_per_chunk)

    def set_telescope_model(self, telescope_model):
        """Sets the telescope model used for the simulation.

        Args:
            telescope_model (oskar.Telescope): Telescope model object.
        """
        _simulator_lib.set_telescope_model(
            self._capsule, telescope_model._capsule)

    def vis_header(self):
        """Returns the visibility header.

        Returns:
            header (oskar.VisHeader): A handle to the visibility header.
        """
        header = VisHeader()
        header._capsule = _simulator_lib.vis_header(self._capsule)
        return header

    def write_block(self, block, block_index):
        """Writes a finalised visibility block.

        This method should be called after a call to finalise_block()
        with the same block index.

        Args:
            block (oskar.VisBlock): The block to write.
            block_index (int): The simulation block index to write.
        """
        _simulator_lib.write_block(self._capsule, block._capsule, block_index)

    # Properties.
    coords_only = property(get_coords_only, set_coords_only)
    num_devices = property(get_num_devices, set_num_devices)
    num_gpus = property(get_num_gpus)
    num_vis_blocks = property(get_num_vis_blocks)

    def _run_blocks(self, thread_id):
        """
        Private method to simulate and process visibility blocks concurrently.

        Each thread executes this function.
        For N devices, there will be N+1 threads.
        Thread 0 is used to finalise the block.
        Threads 1 to N (mapped to compute devices) do the simulation.

        Note that no finalisation is performed on the first iteration (as no
        data are ready yet), and no simulation is performed for the last
        iteration (which corresponds to the last block + 1) as this iteration
        simply finalises the last block.

        Args:
            thread_id (int): Zero-based thread ID.
        """
        # Loop over visibility blocks.
        num_blocks = self.num_vis_blocks
        for b in range(num_blocks + 1):
            # Run simulation in threads 1 to N.
            if thread_id > 0 and b < num_blocks:
                self.run_block(b, thread_id - 1)

            # Finalise and process the previous block in thread 0.
            if thread_id == 0 and b > 0:
                block = self.finalise_block(b - 1)
                self.process_block(block, b - 1)

            # Barrier 1: Reset work unit index.
            self._barrier.wait()
            if thread_id == 0:
                self.reset_work_unit_index()

            # Barrier 2: Synchronise before moving to the next block.
            self._barrier.wait()