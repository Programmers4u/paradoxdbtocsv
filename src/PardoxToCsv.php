<?php

namespace Programmers4u\db\Console\Commands;

use Illuminate\Console\Command;
use Programmers4u\db\Paradox;

class ParadoxToCsv extends Command {
    
    /**
     * The name and signature of the console command.
     *
     * @var string
     */
    protected $signature = 'paradox2csv {source}';

    /**
     * The console command description.
     *
     * @var string
     */
    protected $description = 'Mapping paradox table to csv file.';

    /**
     * Execute the console command.
     *
     * @return mixed
     */
    public function handle()
    {
        
        $this->info('Hi, nice to meet you.');
        $source = $this->argument('source');
        if(empty($source)) {
            $this->alert('Missing source file name');
            exit;
        }
        //$source = "/Users/Macbook/Desktop/pdb/PACJENCI.DB";
        $dest = substr($source,0,strrpos($source,'.')).'.csv';
        $this->paradox($source,$dest);
        exit;



        // start mapping

        // otwórz plik
        $paradox = @file_get_contents($source); 
        if(empty($paradox)) {
            $this->alert('Can\'t open file');
            exit;
        }
        
        // sprawdź nazwy kolumn
        $cols = explode("\x01", $paradox);
        foreach($cols as $col) if(preg_match('/\.DB/isU',$col)) break;

        $col = substr($col,strpos($col,".DB")+3);
        $col = trim(preg_replace("/\\x00+/is",";",$col),'; ');

        //rekordy
        $output = [$col];
        $paradox =  substr($paradox,strpos($paradox,".DB")+3);

        $records = explode("\x80", $paradox);
        $arr = $this->sc();
        foreach($records as $k => $rec) {
            if($k==0) continue;
            $rec = substr($rec,strpos($rec,"\x".$arr[$k-1])+3);
            $rec = trim(preg_replace("/\\x00+/is","^",$rec));
            //$rec = iconv('Windos-1250','UTF-8',$rec); //mb_convert_encoding($rec,'Windows-1250');
            //$rec = preg_replace('/\^.[^a-zA-z0-9 ]+/isU','',$rec);
            //$rec = preg_replace('/\\x09+|\\x0D+|.[^a-zA-z0-9 ]+/is','',$rec);
            $clr = explode("^",$rec);
            $out_crl = [];
            foreach($clr as $cr){
                $cr = trim($cr);
                $cr = '"'.$cr.'"';
                array_push($out_crl,$cr);
            }
            $rec = implode(";",$out_crl);
            if(!empty(trim($rec,'" ')))
                array_push($output,$rec);
            //if($k > 150) break;
        }

        //$col = str_replace("\x00","^",$col);
        //$rec = preg_replace("/\\x00+/is","^",$rec);
        file_put_contents($dest,implode("\n",$output));

    }

    private function sc(){
        $a = ['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'];
        $out = [];
        for($x=0;$x<16;$x++){
            for($z=0;$z<16;$z++) {
            $d=$a[$x][$z];
            array_push($out,$d);
            }
        };
        return $out;
    }

    private function paradox ($source,$dest) {

        $pdx = new Paradox();
        $pdx->m_default_field_value = "?";//" ";
        if ( $pdx->Open($source) ) {
            $tot_rec = $pdx->GetNumRecords();
            if ( $tot_rec ) {
                $output = [];
                $header = '';
                for($rec=0; $rec<$tot_rec; $rec++) {
                     $r = $pdx->GetRecord($rec);
                     //var_dump($r);
                     $poz = '';
                     foreach($r as $k=>$v){
                         if($rec == 0) {
                            $header.= '"'.$k.'";';
                         }
                         $v = preg_replace("/\r|\n/is",' ',$v);
                         $poz.= '"'.$v.'";';
                     }
                     if($rec == 0) {
                        array_push($output,trim($header,";")); 
                     }
                 array_push($output,trim($poz,";"));
                    //break;
                }
                file_put_contents($dest,implode("\n",$output));
            }
            $pdx->Close(); 
        }        
    }

    public function setSourceFile($fileName) 
    {

    }

    public function setDestinationFile($fileName) 
    {

    }

}

?>