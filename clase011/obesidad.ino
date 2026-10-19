/*
  PERCEPTRON OBESIDAD - ARDUINO UNO / WOKWI
  Entrada serie: Edad,Sexo,IMC,Actividad,Calorias,Antecedentes
  0 = sin obesidad; 1 = con obesidad (clasificacion academica)
*/

// PESOS REPORTADOS POR clase011/obesidad.csv (bias primero):
const float W0 = -0.56973729;
const float W1 = 3.1355235;
const float W2 = 1.716018;
const float W3 = 2.2061141;
const float W4 = -3.2898484;
const float W5 = 2.9845651;
const float W6 = 3.9776554;

const float MEDIA_EDAD = 34.734007;
const float MEDIA_SEXO = 0.49158249;
const float MEDIA_IMC = 27.225926;
const float MEDIA_ACTIVIDAD = 3.023569;
const float MEDIA_CALORIAS = 2692.9293;
const float MEDIA_ANTECEDENTES = 0.47811448;

const float ESCALA_EDAD = 12.100865;
const float ESCALA_SEXO = 0.49992914;
const float ESCALA_IMC = 4.0233872;
const float ESCALA_ACTIVIDAD = 2.2983196;
const float ESCALA_CALORIAS = 514.42214;
const float ESCALA_ANTECEDENTES = 0.49952079;

bool numeroValido (String texto)
{

  texto.trim ();
  if (! texto.length ()) return false;

  bool punto = false;
  bool digito = false;

  for (unsigned int i = 0; i < texto.length (); i++)
    {

      char caracter = texto.charAt (i);

      if (caracter >= '0' && caracter <= '9')
        {
          digito = true;
          continue;
        }

      if (caracter == '.' && ! punto)
        {
          punto = true;
          continue;
        }

      return false;
    }

  return digito;
}

void setup ()
{

  Serial.begin (9600);
  Serial.setTimeout (1000);
  Serial.println ("PERCEPTRON OBESIDAD - 6 ENTRADAS");
  Serial.println ("Introduzca: Edad,Sexo,IMC,Actividad,Calorias,Antecedentes");
  Serial.println ("Ejemplo: 34,1,27.8,2,2800,1");
  Serial.println ("Pesos entrenados en clase011/obesidad.csv:");
  Serial.print ("W0 = "); Serial.println (W0, 6);
  Serial.print ("W1 = "); Serial.println (W1, 6);
  Serial.print ("W2 = "); Serial.println (W2, 6);
  Serial.print ("W3 = "); Serial.println (W3, 6);
  Serial.print ("W4 = "); Serial.println (W4, 6);
  Serial.print ("W5 = "); Serial.println (W5, 6);
  Serial.print ("W6 = "); Serial.println (W6, 6);
}

void loop ()
{

  if (! Serial.available ()) return;

  String linea = Serial.readStringUntil ('\n');
  linea.trim ();

  if (! linea.length ()) return;

  String campo [6];
  int inicio = 0;
  bool correcto = true;

  for (int i = 0; i < 6; i++)
    {

      int fin = linea.indexOf (',', inicio);

      if (i < 5 && fin < 0)
        {
          correcto = false;
          break;
        }

      if (i == 5 && fin >= 0)
        {
          correcto = false;
          break;
        }

      campo [i] = i == 5 ? linea.substring (inicio) : linea.substring (inicio, fin);
      campo [i].trim ();

      if (! numeroValido (campo [i]))
        {
          correcto = false;
          break;
        }

      inicio = fin + 1;
    }

  if (! correcto)
    {
      Serial.println ("ERROR. Use: Edad,Sexo,IMC,Actividad,Calorias,Antecedentes");
      return;
    }

  float edad = campo [0].toFloat ();
  float sexo = campo [1].toFloat ();
  float imc = campo [2].toFloat ();
  float actividad = campo [3].toFloat ();
  float calorias = campo [4].toFloat ();
  float antecedentes = campo [5].toFloat ();

  if (edad <= 0 || imc <= 0 || calorias <= 0 || actividad < 0 ||
      ! (sexo == 0 || sexo == 1) || ! (antecedentes == 0 || antecedentes == 1))
    {
      Serial.println ("ERROR: revise los valores; Sexo y Antecedentes deben ser 0 o 1.");
      return;
    }

  float Z = W0
            + W1 * ((edad - MEDIA_EDAD) / ESCALA_EDAD)
            + W2 * ((sexo - MEDIA_SEXO) / ESCALA_SEXO)
            + W3 * ((imc - MEDIA_IMC) / ESCALA_IMC)
            + W4 * ((actividad - MEDIA_ACTIVIDAD) / ESCALA_ACTIVIDAD)
            + W5 * ((calorias - MEDIA_CALORIAS) / ESCALA_CALORIAS)
            + W6 * ((antecedentes - MEDIA_ANTECEDENTES) / ESCALA_ANTECEDENTES);
  int Y = Z >= 0 ? 1 : 0;

  Serial.println ("------------------------------");
  Serial.print ("Suma ponderada Z = "); Serial.println (Z, 6);
  Serial.print ("Salida Y = "); Serial.println (Y);
  if (Y == 1) Serial.println ("CLASIFICACION: CON OBESIDAD (1)");
  else Serial.println ("CLASIFICACION: SIN OBESIDAD (0)");
  Serial.println ("Solo ejercicio academico; NO es diagnostico medico.");
  Serial.println ("Ingrese otro registro:");
}