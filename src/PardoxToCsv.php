<?php

namespace Programmers4u\db\Console\Commands;
use Illuminate\Console\Command;

class ParadoxToCsv extends Commands {
    
    /**
     * The name and signature of the console command.
     *
     * @var string
     */
    protected $signature = 'paradox';

    /**
     * The console command description.
     *
     * @var string
     */
    protected $description = 'Yourcommand test';

    /**
     * Execute the console command.
     *
     * @return mixed
     */
    public function handle()
    {
        $this->info('Hello world!');
    }


    public function setSourceFile($fileName) 
    {

    }

    public function setDestinationFile($fileName) 
    {

    }

}

?>